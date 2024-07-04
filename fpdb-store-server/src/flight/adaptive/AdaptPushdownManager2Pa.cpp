//
// Created by Yifei Yang on 7/4/23.
//

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager2Pa.hpp"
#include "fpdb/store/server/flight/Util.hpp"

namespace fpdb::store::server::flight {

tl::expected<void, std::string> AdaptPushdownManager2Pa::receiveOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) {
  std::unique_lock lock(reqManageMutex_);

  // find adaptive pushdown metrics
  auto adaptPushdownMetricsKeys = generateAdaptPushdownMetricsKeys(req->queryId_, req->op_);
  auto pullupMetricsIt = adaptPushdownMetrics_.find(adaptPushdownMetricsKeys->first);
  if (pullupMetricsIt == adaptPushdownMetrics_.end()) {
    return tl::make_unexpected(fmt::format("Pullup metrics of req '{}-{}' not found", req->queryId_, req->op_));
  }
  auto pushdownMetricsIt = adaptPushdownMetrics_.find(adaptPushdownMetricsKeys->second);
  if (pushdownMetricsIt == adaptPushdownMetrics_.end()) {
    return tl::make_unexpected(fmt::format("Pushdown metrics of req '{}-{}' not found", req->queryId_, req->op_));
  }

  // get pullup time, pushdown time, and compute pa
  int64_t pullupTime = pullupMetricsIt->second;
  int64_t pushdownTime = pushdownMetricsIt->second;
  auto reqPa = std::make_shared<AdaptPushdownReqInfo2Pa>(req);
  reqPa->estPbTime_ = pullupTime;
  reqPa->estPdTime_ = pushdownTime;
  reqPa->pa_ = pullupTime - pushdownTime;

  // process the req, wait if needed
  if (!reqPaQueue_.empty()) {
    // insert into the request queue if not empty, maintain requests sorted by pa
    reqPaQueue_.insert(std::lower_bound(reqPaQueue_.begin(), reqPaQueue_.end(), reqPa, AdaptPushdownReqInfo2PaCmp()),
                       reqPa);
    reqPa->wait_ = true;
  } else {
    // if the request queue is empty
    if (numUsedCpuSlots_ + reqPa->req_->numRequiredCpuCores_ <= MaxThreads && numUsedIoSlots_ < MaxIoThreads) {
      // both CPU and network is available
      reqPa->wait_ = false;
      reqPa->req_->isPushedBack_ = reqPa->estPbTime_ < reqPa->estPdTime_;
    } else if (numUsedCpuSlots_ + reqPa->req_->numRequiredCpuCores_ > MaxThreads && numUsedIoSlots_ >= MaxIoThreads) {
      // neither CPU nor network is available
      reqPa->wait_ = true;
      reqPaQueue_.emplace_back(reqPa);
    } else if (numUsedIoSlots_ >= MaxIoThreads) {
      // only CPU is available
      auto expPbMaxRemainTime = maxRunningRemainTime(false);
      if (!expPbMaxRemainTime.has_value()) {
        return tl::make_unexpected(expPbMaxRemainTime.error());
      }
      if (reqPa->estPdTime_ <= reqPa->estPbTime_ + *expPbMaxRemainTime) {
        reqPa->wait_ = false;
        reqPa->req_->isPushedBack_ = false;
      } else {
        reqPa->wait_ = true;
        reqPaQueue_.emplace_back(reqPa);
      }
    } else {
      // only network is available
      auto expPbMaxRemainTime = maxRunningRemainTime(true);
      if (!expPbMaxRemainTime.has_value()) {
        return tl::make_unexpected(expPbMaxRemainTime.error());
      }
      if (reqPa->estPbTime_ <= reqPa->estPdTime_ + *expPbMaxRemainTime) {
        reqPa->wait_ = false;
        reqPa->req_->isPushedBack_ = true;
      } else {
        reqPa->wait_ = true;
        reqPaQueue_.emplace_back(reqPa);
      }
    }
  }

  // wait or admit
  reqPa->cv_.wait(lock, [&]{
    return !reqPa->wait_;
  });
  admitOne(reqPa);

  return {};
}

void AdaptPushdownManager2Pa::finishOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) {
  std::unique_lock lock(reqManageMutex_);
  if (req->isPushedBack_) {
    pbReqSet_.erase(req);
    --numUsedIoSlots_;
  } else {
    pdReqSet_.erase(req);
    numUsedCpuSlots_ -= req->numRequiredCpuCores_;
  }

  // examine and process the head and tail of the req queue
  process();
}

tl::expected<void, std::string> AdaptPushdownManager2Pa::process() {
  // divide reqQueue into pushdown portion and pushback portion, such that the runtime of both is roughly equal
  int64_t pdTotalTime = 0, pbTotalTime = 0;
  auto currTime = std::chrono::steady_clock::now();
  for (const auto &req: pdReqSet_) {
    auto expRemainTime = runningRemainTime(req, currTime);
    if (!expRemainTime.has_value()) {
      return tl::make_unexpected(expRemainTime.error());
    }
    pdTotalTime += *expRemainTime;
  }
  for (const auto &req: pbReqSet_) {
    auto expRemainTime = runningRemainTime(req, currTime);
    if (!expRemainTime.has_value()) {
      return tl::make_unexpected(expRemainTime.error());
    }
    pbTotalTime += *expRemainTime;
  }
  auto leftIt = reqPaQueue_.begin(), rightIt = reqPaQueue_.end() - 1;
  while (leftIt < rightIt) {
    if (pbTotalTime < pdTotalTime) {
      pbTotalTime += (*leftIt)->estPbTime_;
      ++leftIt;
    } else {
      pdTotalTime += (*rightIt)->estPdTime_;
      --rightIt;
    }
  }
  // decide for the border element
  if (pbTotalTime < pdTotalTime) {
    ++rightIt;
  } else {
    --leftIt;
  }

  // examine pushdown path, from end to rightIt
  auto pdAdmitLeftIt = reqPaQueue_.end() - 1;
  while (pdAdmitLeftIt >= rightIt) {
    const auto &req = *pdAdmitLeftIt;
    if (numUsedCpuSlots_ + req->req_->numRequiredCpuCores_ > MaxThreads) {
      break;
    }
    req->req_->isPushedBack_ = false;
    admitOne(req);
    --pdAdmitLeftIt;
  }
  reqPaQueue_.erase(pdAdmitLeftIt + 1, reqPaQueue_.end());

  // examine pushback path, from begin to leftIt
  auto pbAdmitRightIt = reqPaQueue_.begin();
  while (pbAdmitRightIt <= leftIt) {
    if (numUsedIoSlots_ >= MaxIoThreads) {
      break;
    }
    const auto &req = *pdAdmitLeftIt;
    req->req_->isPushedBack_ = true;
    admitOne(req);
    ++pbAdmitRightIt;
  }
  reqPaQueue_.erase(reqPaQueue_.begin(), pbAdmitRightIt);
  
  return {};
}

void AdaptPushdownManager2Pa::admitOne(const std::shared_ptr<AdaptPushdownReqInfo2Pa> &reqPa) {
  if (reqPa->req_->isPushedBack_) {
    reqPa->req_->estExecTime_ = reqPa->estPbTime_;
    pbReqSet_.emplace(reqPa->req_);
    ++numUsedIoSlots_;
  } else {
    reqPa->req_->estExecTime_ = reqPa->estPdTime_;
    pdReqSet_.emplace(reqPa->req_);
    numUsedCpuSlots_ += reqPa->req_->numRequiredCpuCores_;
  }
  reqPa->wait_ = false;
  reqPa->req_->startTime_ = std::chrono::steady_clock::now();
  reqPa->cv_.notify_one();
}

tl::expected<int64_t, std::string>
AdaptPushdownManager2Pa::runningRemainTime(const std::shared_ptr<AdaptPushdownReqInfo2> &req,
                                           const std::chrono::steady_clock::time_point &now) {
  if (!req->estExecTime_.has_value()) {
    return tl::make_unexpected(fmt::format("Estimated execution time of req '{}-{}' not set",
                                           req->queryId_, req->op_));
  }
  if (!req->startTime_.has_value()) {
    return tl::make_unexpected(fmt::format("Start time of req '{}-{}' not set",
                                           req->queryId_, req->op_));
  }
  return std::max((int64_t) 0, (int64_t) (*req->estExecTime_ -
      std::chrono::duration_cast<std::chrono::nanoseconds>(now - *req->startTime_).count()));
}

tl::expected<int64_t, std::string> AdaptPushdownManager2Pa::maxRunningRemainTime(bool isPushdown) {
  const auto &reqSet = isPushdown ? pdReqSet_ : pbReqSet_;
  int64_t maxRemainTime = 0;
  auto currTime = std::chrono::steady_clock::now();
  for (const auto &req: reqSet) {
    auto expRemainTime = runningRemainTime(req, currTime);
    if (!expRemainTime.has_value()) {
      return tl::make_unexpected(expRemainTime.error());
    }
    maxRemainTime = std::max(maxRemainTime, *expRemainTime);
  }
  return maxRemainTime;
}

}

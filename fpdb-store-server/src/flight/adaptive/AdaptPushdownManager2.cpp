//
// Created by Yifei Yang on 7/4/23.
//

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager2.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fpdb/executor/physical/Globals.h"

namespace fpdb::store::server::flight {

void AdaptPushdownManager2::addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other) {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.insert(other.begin(), other.end());
}

void AdaptPushdownManager2::clearAdaptPushdownMetrics() {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.clear();
}

tl::expected<void, std::string> AdaptPushdownManager2::receiveOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) {
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

  // get pullup time, pushdown time, and wait time
  int64_t pullupTime = pullupMetricsIt->second;
  int64_t pushdownTime = pushdownMetricsIt->second;
  auto expPdWaitTime = getWaitTime(req, true);
  if (!expPdWaitTime.has_value()) {
    return tl::make_unexpected(expPdWaitTime.error());
  }
  auto expPbWaitTime = getWaitTime(req, false);
  if (!expPbWaitTime.has_value()) {
    return tl::make_unexpected(expPbWaitTime.error());
  }

  // make decision
  bool execAsPushdown = (pullupTime + *expPbWaitTime) > (pushdownTime + *expPdWaitTime);
  req->isPushedBack_ = !execAsPushdown;
  req->estExecTime_ = execAsPushdown ? pushdownTime : pullupTime;

  // process the req, wait if needed
  if (req->isPushedBack_) {
    pbReqSet_.emplace(req);
    pbCv_.wait(lock, [&]{
      return numUsedIoSlots_ < MaxIoThreads;
    });
    ++numUsedIoSlots_;
  } else {
    pdReqSet_.emplace(req);
    pdCv_.wait(lock, [&]{
      return numUsedCpuSlots_ < MaxThreads;
    });
    numUsedCpuSlots_ += req->numRequiredCpuCores_;
  }
  req->startTime_ = std::chrono::steady_clock::now();
  return {};
}

void AdaptPushdownManager2::finishOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) {
  std::unique_lock lock(reqManageMutex_);
  if (req->isPushedBack_) {
    pbReqSet_.erase(req);
    --numUsedIoSlots_;
    pbCv_.notify_one();
  } else {
    pdReqSet_.erase(req);
    numUsedCpuSlots_ -= req->numRequiredCpuCores_;
    pdCv_.notify_one();
  }
}

tl::expected<int64_t, std::string>
AdaptPushdownManager2::getWaitTime(const std::shared_ptr<AdaptPushdownReqInfo2>&, bool isPushdown) {
  const std::unordered_set<std::shared_ptr<AdaptPushdownReqInfo2>, AdaptPushdownReqInfo2PtrHash,
      AdaptPushdownReqInfo2PtrPred> &reqSet = isPushdown ? pdReqSet_ : pbReqSet_;
  int64_t waitTime = 0;
  auto currTime = std::chrono::steady_clock::now();
  for (const auto &existReq: reqSet) {
    if (!existReq->estExecTime_.has_value()) {
      return tl::make_unexpected(fmt::format("Estimated execution time of req '{}-{}' not set",
                                             existReq->queryId_, existReq->op_));
    }
    if (existReq->startTime_.has_value()) {
      waitTime += std::max((int64_t) 0, (int64_t) (*existReq->estExecTime_ -
          std::chrono::duration_cast<std::chrono::nanoseconds>(currTime - *existReq->startTime_).count()));
    } else {
      waitTime += *existReq->estExecTime_;
    }
  }
  // calibrate the formula
  double defaultTuneFactor = 5.0;
  size_t pbSetSize = pbReqSet_.size(), pdSetSize = pdReqSet_.size();
  double pdTuneFactor = std::max(defaultTuneFactor * pdSetSize / (pdSetSize + pbSetSize), 1.0);
  if (isPushdown) {
    return (waitTime * 1.0 / MaxThreads) / pdTuneFactor;
  } else {
    return waitTime * 1.0 / MaxIoThreads;
  }
}

tl::expected<std::pair<std::string, std::string>, std::string>
AdaptPushdownManager2::generateAdaptPushdownMetricsKeys(long queryId, const std::string &op) {
  if (op.substr(0, fpdb::executor::physical::PushdownOpNamePrefix.size()) ==
      fpdb::executor::physical::PushdownOpNamePrefix) {
    auto pushdownMetricsKey = fmt::format("{}-{}", queryId, op);
    std::string pullupMetricsKey = pushdownMetricsKey;
    pullupMetricsKey.replace(pullupMetricsKey.find(fpdb::executor::physical::PushdownOpNamePrefix),
                             fpdb::executor::physical::PushdownOpNamePrefix.size(),
                             fpdb::executor::physical::PullupOpNamePrefix);
    return std::make_pair(pullupMetricsKey, pushdownMetricsKey);
  } else {
    return tl::make_unexpected(fmt::format("Invalid op name for adaptive pushdown metrics: {}", op));
  }
}

}

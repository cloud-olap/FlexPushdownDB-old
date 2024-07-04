//
// Created by Yifei Yang on 9/18/23.
//

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager3.hpp"
#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager3Pa.hpp"
#include "fpdb/store/server/flight/adaptive/PushbackDoubleExecCmd.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fpdb/executor/physical/Globals.h"

namespace fpdb::store::server::flight {

void AdaptPushdownManager3::addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other) {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.insert(other.begin(), other.end());
}

void AdaptPushdownManager3::clearAdaptPushdownMetrics() {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.clear();
}

tl::expected<void, std::string> AdaptPushdownManager3::receiveOne(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  std::unique_lock lock(reqMutex_);

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

  // compute PA if needed
  if (EnablePaAwareAdaptPushdown) {
    reinterpret_cast<AdaptPushdownManager3Pa*>(this)->computePa(
            req, pullupMetricsIt->second, pushdownMetricsIt->second);
  }

  // for regular req
  if (!req->extraInfo_.isDoubleExec_) {
    // if wait queue is not empty, then directly append into the queue, otherwise evaluate for execution
    if (reqQueue_.empty()) {
      // check resource
      // check first path, faster one between pushdown and pushback
      bool isPdFaster = pushdownMetricsIt->second <= pullupMetricsIt->second;
      if (isPdFaster) {
        if (numUsedCpuSlots_ < MaxThreads) {
          req->isPushedBack_ = false;
          // update state of this req
          preExec(req);
          return {};
        }
      } else {
        if (numUsedIoSlots_ < MaxIoThreads) {
          req->isPushedBack_ = true;
          // update state of this req
          preExec(req);
          return {};
        }
      }
      // check second path, slower one between pushdown and pushback
      if (isPdFaster) {
        if (numUsedIoSlots_ < MaxIoThreads) {
          req->isPushedBack_ = true;
          // update state of this req
          preExec(req);
          return {};
        }
      } else {
        if (numUsedCpuSlots_ < MaxThreads) {
          req->isPushedBack_ = false;
          // update state of this req
          preExec(req);
          return {};
        }
      }
    }

    // either wait queue is not empty or no slots in both paths, have to wait
    auto cv = std::make_shared<std::condition_variable_any>();
    reqCvs_[req] = cv;
    enqueue(req);
    cv->wait(lock, [&]{
      if (reqCvs_.find(req) != reqCvs_.end()) {     // this means req has not been popped out of the wait queue
        return false;
      }
      return true;
    });
  }

  // for double-exec req
  else {
    // wait if either regular wait queue or double-exec is not empty
    if (reqQueue_.empty() && doubleExecReqQueue_.empty()) {
      // check resource, double-exec req can only run as pushdown
      req->isPushedBack_ = false;
      if (numUsedCpuSlots_ < MaxThreads) {
        // update state of this req
        preExec(req);
        return {};
      }
    }

    // wait in double-exec wait queue
    auto cv = std::make_shared<std::condition_variable_any>();
    doubleExecReqCvs_[req] = cv;
    enqueue(req);
    cv->wait(lock, [&]{
      if (doubleExecReqCvs_.find(req) != doubleExecReqCvs_.end()) {     // this means req has not been popped out of the wait queue
        return false;
      }
      return true;
    });
  };

  return {};
}

void AdaptPushdownManager3::finishOne(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  std::unique_lock lock(reqMutex_);

  // update state of this req
  postExec(req);

  // find the next req to exec if any
  // first check regular req queue
  std::shared_ptr<AdaptPushdownReqInfo3> nextReq = nullptr;
  std::shared_ptr<std::condition_variable_any> nextReqCv = nullptr;
  if (!reqQueue_.empty()) {
    nextReq = dequeue(req->isPushedBack_);
    nextReqCv = reqCvs_[nextReq];
    reqCvs_.erase(nextReq);
  }

  // if not found, then check double-exec req queue
  if (nextReq == nullptr && EnablePushbackTailReqDoubleExec) {
    if (!doubleExecReqQueue_.empty()) {
      // double-exec can only be run as pushdown, need to check resource
      if (numUsedCpuSlots_ < MaxThreads) {
        nextReq = dequeue(false, true);
        nextReqCv = doubleExecReqCvs_[nextReq];
        doubleExecReqCvs_.erase(nextReq);
      }
    }
  }

  // let next req run if we find one
  if (nextReq != nullptr) {
    preExec(nextReq);
    nextReqCv->notify_one();
  }
}

void AdaptPushdownManager3::setNumReqToTail(int64_t numReqToTail) {
  numReqToTail_ = numReqToTail;
}

void AdaptPushdownManager3::preExec(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  if (req->isPushedBack_) {
    pbExecSet_.emplace(req);
    ++numUsedIoSlots_;
  } else {
    pdExecSet_.emplace(req);
    ++numUsedCpuSlots_;
  }

  // try to check if we reach the tail reqs, do not check for double-exec
  if (req->extraInfo_.isDoubleExec_) {
    return;
  }
  if (EnablePushbackTailReqDoubleExec) {
    ++numReqReceived_;
    if (numReqToTail_.has_value() && numReqReceived_ == *numReqToTail_) {
      tryPushbackDoubleExec();
    }
  }
}

void AdaptPushdownManager3::postExec(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  if (req->isPushedBack_) {
    pbExecSet_.erase(req);
    --numUsedIoSlots_;
  } else {
    pdExecSet_.erase(req);
    --numUsedCpuSlots_;
  }
}

void AdaptPushdownManager3::enqueue(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  if (!req->extraInfo_.isDoubleExec_) {
    reqQueue_.push_back(req);
  } else {
    doubleExecReqQueue_.push_back(req);
  }
}

std::shared_ptr<AdaptPushdownReqInfo3>
AdaptPushdownManager3::dequeue(bool isPushedBack, bool isDoubleExecReqQueue) {
  auto &reqQueue = isDoubleExecReqQueue ? doubleExecReqQueue_ : reqQueue_;
  auto nextReq = reqQueue.front();
  nextReq->isPushedBack_ = isPushedBack;
  reqQueue.pop_front();
  return nextReq;
}

tl::expected<std::pair<std::string, std::string>, std::string>
AdaptPushdownManager3::generateAdaptPushdownMetricsKeys(long queryId, const std::string &op) {
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

void AdaptPushdownManager3::tryPushbackDoubleExec() {
  // loop over each running pushback exec
  for (const auto &req: pbExecSet_) {
    // skip those already being double-exec
    if (req->extraInfo_.isDoubleExecFrom_) {
      continue;
    }
    
    // send request to the sender compute node
    auto client = computeFlightClients_.getFlightClient(req->extraInfo_.senderIp_, req->extraInfo_.senderPort_);
    auto cmdObj = PushbackDoubleExecCmd::make(req->queryId_, req->op_);
    auto expCmd = cmdObj->serialize(false);
    if (!expCmd.has_value()) {
      printf("[Pushback double-exec failed] %s\n", expCmd.error().c_str());
      continue;
    }
    auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
    std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
    std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
    auto status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
    if (!status.ok()) {
      printf("[Pushback double-exec failed] %s\n", status.message().c_str());
      continue;
    }
    status = writer->Close();
    if (!status.ok()) {
      printf("[Pushback double-exec failed] %s\n", status.message().c_str());
      continue;
    }

    // mark the req has a double-exec fork
    req->extraInfo_.isDoubleExecFrom_ = true;
  }

  // reset tail status
  numReqToTail_ = std::nullopt;
  numReqReceived_ = 0;
}

}

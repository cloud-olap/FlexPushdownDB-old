//
// Created by Yifei Yang on 10/31/22.
//

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager.hpp"
#include "fpdb/store/server/flight/Util.hpp"
#include "fpdb/executor/physical/Globals.h"
#include "fpdb/util/CPUMonitor.h"
#include "cmath"

namespace fpdb::store::server::flight {

void AdaptPushdownManager::addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other) {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.insert(other.begin(), other.end());
}

void AdaptPushdownManager::clearAdaptPushdownMetrics() {
  std::unique_lock lock(metricsMutex_);
  adaptPushdownMetrics_.clear();
}

tl::expected<bool, std::string> AdaptPushdownManager::receiveOne(const std::shared_ptr<AdaptPushdownReqInfo> &req) {
  std::unique_lock lock(reqManageMutex_);

  // find adaptive pushdown metrics
  auto adaptPushdownMetricsKeys = generateAdaptPushdownMetricsKeys(req->queryId_, req->op_);
  auto pullupMetricsIt = adaptPushdownMetrics_.find(adaptPushdownMetricsKeys->first);
  auto pushdownMetricsIt = adaptPushdownMetrics_.find(adaptPushdownMetricsKeys->second);
  if (pullupMetricsIt == adaptPushdownMetrics_.end()) {
    return tl::make_unexpected(fmt::format("Pullup metrics of req '{}-{}' not found", req->queryId_, req->op_));
  }
  if (pushdownMetricsIt == adaptPushdownMetrics_.end()) {
    return tl::make_unexpected(fmt::format("Pushdown metrics of req '{}-{}' not found", req->queryId_, req->op_));
  }

  // get pullup time, pushdown time, and wait time
  int64_t pullupTime = pullupMetricsIt->second;
  req->estExecTime_ = pushdownMetricsIt->second;
  auto expWaitTime = getWaitTime(req);
  if (!expWaitTime.has_value()) {
    return tl::make_unexpected(expWaitTime.error());
  }

  // Compute weight of pullup time. Since we cannot guarantee to dedicate a fix amount of network bandwidth when
  // there are too many pullup requests, we multiply the pullup time with a weight which is proportional to the number
  // of reqs that are pushed back.
  // This simulates the sharing of the network bandwidth when there are more reqs that are pushed back than
  // NumMaxPullupReqs. When there are no more reqs that are pushed back than NumMaxPullupReqs, each pushed back req can
  // get its dedicated network bandwidth, then the weight is 1.
  // Use "ceil" instead of float weight because we assume the newly pushed back reqs don't affect the weight of the
  // previous ones, since we cannot change past pushdown decisions.
  int pullupWeight = std::max(1, (int) std::ceil(numFallBackReqs_ * 1.0 / MaxIoThreads));

  // check if need to fall back to pullup
  bool execAsPushdown = ((pullupWeight * pullupTime) > (*req->estExecTime_ + *expWaitTime));
  if (!execAsPushdown) {
    ++numFallBackReqs_;
  }
  return execAsPushdown;
}

void AdaptPushdownManager::admitOne(const std::shared_ptr<AdaptPushdownReqInfo> &req) {
  std::unique_lock lock(reqManageMutex_);
  reqSet_.emplace(req);
  reqManageCv_.wait(lock, [&] {
    return numUsedThreads_ < MaxThreads;
  });
  req->startTime_ = std::chrono::steady_clock::now();
  numUsedThreads_ += req->numRequiredCpuCores_;
}

void AdaptPushdownManager::finishOne(const std::shared_ptr<AdaptPushdownReqInfo> &req) {
  std::unique_lock lock(reqManageMutex_);
  reqSet_.erase(req);
  numUsedThreads_ -= req->numRequiredCpuCores_;
  reqManageCv_.notify_one();
}

void AdaptPushdownManager::clearNumFallBackReqs() {
  numFallBackReqs_ = 0;
}

tl::expected<int64_t, std::string> AdaptPushdownManager::getWaitTime(const std::shared_ptr<AdaptPushdownReqInfo> &req) {
  int64_t waitTime = 0;
  auto currTime = std::chrono::steady_clock::now();
  for (const auto &existReq: reqSet_) {
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
  return (waitTime * 1.0 / MaxThreads) * (req->numRequiredCpuCores_ * 1.0 / MaxThreads);
}

tl::expected<std::pair<std::string, std::string>, std::string>
AdaptPushdownManager::generateAdaptPushdownMetricsKeys(long queryId, const std::string &op) {
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

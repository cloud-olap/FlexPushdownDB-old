//
// Created by Yifei Yang on 10/31/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER_HPP

#include "tl/expected.hpp"
#include "fmt/format.h"
#include "unordered_map"
#include "unordered_set"
#include "vector"
#include "string"
#include "mutex"
#include "condition_variable"
#include "thread"

namespace fpdb::store::server::flight {

struct AdaptPushdownReqInfo {
  AdaptPushdownReqInfo(long queryId,
                       const std::string &op,
                       int numRequiredCpuCores):
  queryId_(queryId), op_(op), numRequiredCpuCores_(numRequiredCpuCores) {}

  long queryId_;
  std::string op_;
  int numRequiredCpuCores_;
  std::optional<std::chrono::steady_clock::time_point> startTime_ = std::nullopt;
  std::optional<int64_t> estExecTime_ = std::nullopt;
};

struct AdaptPushdownReqInfoPointerHash {
  inline size_t operator()(const std::shared_ptr<AdaptPushdownReqInfo> &req) const {
    return std::hash<std::string>()(fmt::format("{}-{}", std::to_string(req->queryId_), req->op_));
  }
};

struct AdaptPushdownReqInfoPointerPredicate {
  inline bool operator()(const std::shared_ptr<AdaptPushdownReqInfo> &req1,
                         const std::shared_ptr<AdaptPushdownReqInfo> &req2) const {
    return req1->queryId_ == req2->queryId_ && req1->op_ == req2->op_;
  }
};

class AdaptPushdownManager {

public:
  AdaptPushdownManager() = default;

  // Save adaptive pushdown metrics
  void addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other);

  // Clear adaptive pushdown metrics
  void clearAdaptPushdownMetrics();

  // process an incoming pushdown request, return "true" to execute as pushdown, "false" to fall back as pullup
  tl::expected<bool, std::string> receiveOne(const std::shared_ptr<AdaptPushdownReqInfo> &req);

  // admit executing one pushdown request (may wait due to non-enough resource)
  void admitOne(const std::shared_ptr<AdaptPushdownReqInfo> &req);

  // when one request is finished
  void finishOne(const std::shared_ptr<AdaptPushdownReqInfo> &req);

  void clearNumFallBackReqs();

private:
  // get the estimated wait time to execute the req at this point
  tl::expected<int64_t, std::string> getWaitTime(const std::shared_ptr<AdaptPushdownReqInfo> &req);

  // generate adaptive pushdown metrics keys for both pullup metrics and pushdown metrics
  static tl::expected<std::pair<std::string, std::string>, std::string>
  generateAdaptPushdownMetricsKeys(long queryId, const std::string &op);

  // for adaptive pushdown metrics
  std::unordered_map<std::string, int64_t> adaptPushdownMetrics_;
  std::mutex metricsMutex_;

  // for managing pushdown requests adaptively
  std::unordered_set<std::shared_ptr<AdaptPushdownReqInfo>, AdaptPushdownReqInfoPointerHash,
      AdaptPushdownReqInfoPointerPredicate> reqSet_;   // req set, contain both running and waiting reqs
  std::mutex reqManageMutex_;
  std::condition_variable_any reqManageCv_;
  int numUsedThreads_ = 0;

  // for computing weight of pullup time, since we cannot dedicate a fix amount of network bandwidth when there are
  // too many pullup requests
  int numFallBackReqs_ = 0;
};

}


#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER_HPP

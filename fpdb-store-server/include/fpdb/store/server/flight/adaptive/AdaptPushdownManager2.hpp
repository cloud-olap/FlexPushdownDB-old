//
// Created by Yifei Yang on 7/4/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2_HPP

#include "tl/expected.hpp"
#include "fmt/format.h"
#include "string"
#include "unordered_map"
#include "unordered_set"
#include "mutex"
#include "condition_variable"
#include "optional"

namespace fpdb::store::server::flight {

struct AdaptPushdownReqInfo2 {
  AdaptPushdownReqInfo2(long queryId,
                        const std::string &op,
                        int numRequiredCpuCores = 1):
    queryId_(queryId), op_(op), numRequiredCpuCores_(numRequiredCpuCores) {}

  long queryId_;
  std::string op_;
  int numRequiredCpuCores_;
  std::optional<std::chrono::steady_clock::time_point> startTime_ = std::nullopt;
  std::optional<int64_t> estExecTime_ = std::nullopt;
  bool isPushedBack_;
};

struct AdaptPushdownReqInfo2PtrHash {
  inline size_t operator()(const std::shared_ptr<AdaptPushdownReqInfo2> &req) const {
    return std::hash<std::string>()(fmt::format("{}-{}", std::to_string(req->queryId_), req->op_));
  }
};

struct AdaptPushdownReqInfo2PtrPred {
  inline bool operator()(const std::shared_ptr<AdaptPushdownReqInfo2> &req1,
                         const std::shared_ptr<AdaptPushdownReqInfo2> &req2) const {
    return req1->queryId_ == req2->queryId_ && req1->op_ == req2->op_;
  }
};

/**
 * 2 queue version of the vanilla AdaptPushdownManager
 */
class AdaptPushdownManager2 {
public:
  AdaptPushdownManager2() = default;

  // Save adaptive pushdown metrics
  void addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other);

  // Clear adaptive pushdown metrics
  void clearAdaptPushdownMetrics();

  // process an incoming pushdown request on receiving, decide pushdown/pushback, wait or not
  virtual tl::expected<void, std::string> receiveOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req);

  // when one request is finished
  virtual void finishOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req);

protected:
  // get the estimated wait time to execute the req at this point, for both pushdown and pushback
  tl::expected<int64_t, std::string> getWaitTime(const std::shared_ptr<AdaptPushdownReqInfo2> &req, bool isPushdown);

  // generate adaptive pushdown metrics keys for both pullup metrics and pushdown metrics
  static tl::expected<std::pair<std::string, std::string>, std::string>
  generateAdaptPushdownMetricsKeys(long queryId, const std::string &op);

  // for adaptive pushdown metrics
  std::unordered_map<std::string, int64_t> adaptPushdownMetrics_;
  std::mutex metricsMutex_;

  // monitor cpu and network resources for pushback and pushdown respectively
  int numUsedCpuSlots_ = 0;   // max: MaxThreads
  int numUsedIoSlots_ = 0;    // max: MaxIoThreads

  // req sets, contain both running and waiting reqs for AdaptPushdownManager2
  // contain only running reqs for AdaptPushdownManager2Pa, where the waiting requests are stored in reqQueue
  std::unordered_set<std::shared_ptr<AdaptPushdownReqInfo2>, AdaptPushdownReqInfo2PtrHash, AdaptPushdownReqInfo2PtrPred>
      pdReqSet_, pbReqSet_;

  // mutex for manipulating states
  std::mutex reqManageMutex_;

private:
  // for wait mechanism
  std::condition_variable_any pdCv_, pbCv_;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2_HPP

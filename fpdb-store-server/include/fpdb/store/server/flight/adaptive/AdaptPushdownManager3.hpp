//
// Created by Yifei Yang on 9/18/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3_HPP

#include "ReqExtraInfo.hpp"
#include "fpdb/executor/flight/FlightClients.h"
#include "tl/expected.hpp"
#include "fmt/format.h"
#include "unordered_map"
#include "unordered_set"
#include "string"
#include "deque"
#include "condition_variable"

namespace fpdb::store::server::flight {

struct AdaptPushdownReqInfo3 {
  AdaptPushdownReqInfo3(long queryId,
                        const std::string &op,
                        const ReqExtraInfo &extraInfo):
    queryId_(queryId), op_(op), extraInfo_(extraInfo) {}

  long queryId_;
  std::string op_;
  bool isPushedBack_;

  // used for tail req double exec
  ReqExtraInfo extraInfo_;
};

// for double-exec, only the original one enters resource management
struct AdaptPushdownReqInfo3PtrHash {
  inline size_t operator()(const std::shared_ptr<AdaptPushdownReqInfo3> &req) const {
    return std::hash<std::string>()(fmt::format("{}-{}", req->queryId_, req->op_));
  }
};

struct AdaptPushdownReqInfo3PtrPred {
  inline bool operator()(const std::shared_ptr<AdaptPushdownReqInfo3> &req1,
                         const std::shared_ptr<AdaptPushdownReqInfo3> &req2) const {
    return req1->queryId_ == req2->queryId_ && req1->op_ == req2->op_;
  }
};

/**
 * 1 unified queue (for both pushdown and pushback) design of AdaptPushdownManager
 */
class AdaptPushdownManager3 {
public:
  AdaptPushdownManager3() = default;
  virtual ~AdaptPushdownManager3() = default;

  // save adaptive pushdown metrics
  void addAdaptPushdownMetrics(const std::unordered_map<std::string, int64_t> &other);

  // clear adaptive pushdown metrics
  void clearAdaptPushdownMetrics();

  // receive an incoming pushdown request, the request can be put on execution when either pushdown or pushback has
  // idle slots, otherwise wait in the unified queue
  tl::expected<void, std::string> receiveOne(const std::shared_ptr<AdaptPushdownReqInfo3> &req);

  // when one request is finished
  void finishOne(const std::shared_ptr<AdaptPushdownReqInfo3> &req);

  // set the point of tail reqs
  void setNumReqToTail(int64_t numReqToTail);

protected:
  // manage the wait queue, default impl is FIFO
  virtual void enqueue(const std::shared_ptr<AdaptPushdownReqInfo3> &req);
  virtual std::shared_ptr<AdaptPushdownReqInfo3> dequeue(bool isPushedBack, bool isDoubleExecReqQueue = false);

private:
  // some setup just before and after exec, assume lock has been assured by caller
  void preExec(const std::shared_ptr<AdaptPushdownReqInfo3> &req);
  void postExec(const std::shared_ptr<AdaptPushdownReqInfo3> &req);

  // generate adaptive pushdown metrics keys for both pullup metrics and pushdown metrics
  static tl::expected<std::pair<std::string, std::string>, std::string>
  generateAdaptPushdownMetricsKeys(long queryId, const std::string &op);

  // try to start another pushdown exec for the tail pushback req
  void tryPushbackDoubleExec();

  // for adaptive pushdown metrics
  std::unordered_map<std::string, int64_t> adaptPushdownMetrics_;
  std::mutex metricsMutex_;

  // monitor cpu and network resources for pushback and pushdown respectively
  int numUsedCpuSlots_ = 0;   // max: MaxThreads
  int numUsedIoSlots_ = 0;    // max: MaxIoThreads

  // used for managing the req execution
  using ReqQueue = std::deque<std::shared_ptr<AdaptPushdownReqInfo3>>;
  using ReqCvMap = std::unordered_map<std::shared_ptr<AdaptPushdownReqInfo3>,
          std::shared_ptr<std::condition_variable_any>, AdaptPushdownReqInfo3PtrHash, AdaptPushdownReqInfo3PtrPred>;
  using ReqSet = std::unordered_set<std::shared_ptr<AdaptPushdownReqInfo3>, AdaptPushdownReqInfo3PtrHash,
          AdaptPushdownReqInfo3PtrPred>;
  std::mutex reqMutex_;
  ReqCvMap reqCvs_;

private:
  // used for pushback double-exec
  ReqSet pdExecSet_, pbExecSet_;
  ReqCvMap doubleExecReqCvs_;
  // let us know where we reach the tail reqs, maybe better make one per query
  std::optional<int64_t> numReqToTail_ = std::nullopt;
  int64_t numReqReceived_ = 0;

  // clients to connect the compute nodes
  executor::flight::FlightClients computeFlightClients_;

protected:
  ReqQueue reqQueue_;
  ReqQueue doubleExecReqQueue_;    // a separate queue to manage double-exec reqs
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3_HPP

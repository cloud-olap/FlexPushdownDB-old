//
// Created by Yifei Yang on 7/4/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2PA_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2PA_HPP

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager2.hpp"
#include "deque"

namespace fpdb::store::server::flight {

/**
 * Wrap the req base by adding pushdown amenability
 */
struct AdaptPushdownReqInfo2Pa {
  AdaptPushdownReqInfo2Pa(const std::shared_ptr<AdaptPushdownReqInfo2> &req):
    req_(req) {}

  std::shared_ptr<AdaptPushdownReqInfo2> req_;
  int64_t estPdTime_, estPbTime_;
  int64_t pa_;          // pushdown amenability = estPbTime - estPdTime
  bool wait_;
  std::condition_variable_any cv_;
};

struct AdaptPushdownReqInfo2PaPtrHash {
  inline size_t operator()(const std::shared_ptr<AdaptPushdownReqInfo2Pa> &req) const {
    return AdaptPushdownReqInfo2PtrHash()(req->req_);
  }
};

struct AdaptPushdownReqInfo2PaPtrPred {
  inline bool operator()(const std::shared_ptr<AdaptPushdownReqInfo2Pa> &req1,
                         const std::shared_ptr<AdaptPushdownReqInfo2Pa> &req2) const {
    return AdaptPushdownReqInfo2PtrPred()(req1->req_, req2->req_);
  }
};

struct AdaptPushdownReqInfo2PaCmp {
  inline bool operator()(const std::shared_ptr<AdaptPushdownReqInfo2Pa> &req1,
                  const std::shared_ptr<AdaptPushdownReqInfo2Pa> &req2) {
    return req1->pa_ - req2->pa_;
  }
};

/**
 * Pushdown amenability-aware version of AdaptPushdownManager2
 */
class AdaptPushdownManager2Pa: public AdaptPushdownManager2 {
public:
  AdaptPushdownManager2Pa() = default;

  // process an incoming pushdown request on receiving
  // pushdown decisions are delayed until either CPU or network resources are not both saturated
  tl::expected<void, std::string> receiveOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) override;

  // when one request is finished
  void finishOne(const std::shared_ptr<AdaptPushdownReqInfo2> &req) override;

private:
  // examine the double ended request queue for both pushdown and pushback execution
  // process as much as possible if there is enough resource
  // called when there is req finished (by finishOne())
  tl::expected<void, std::string> process();

  // admit one request for execution, lock should be maintained by caller
  void admitOne(const std::shared_ptr<AdaptPushdownReqInfo2Pa> &reqPa);

  // get the remaining time of a running request
  tl::expected<int64_t, std::string> runningRemainTime(const std::shared_ptr<AdaptPushdownReqInfo2> &req,
                                                       const std::chrono::steady_clock::time_point &now);

  // get the max remaining time of the running requests
  tl::expected<int64_t, std::string> maxRunningRemainTime(bool isPushdown);

  // unified request queue sorted by pushdown amenability
  std::deque<std::shared_ptr<AdaptPushdownReqInfo2Pa>> reqPaQueue_;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER2PA_HPP

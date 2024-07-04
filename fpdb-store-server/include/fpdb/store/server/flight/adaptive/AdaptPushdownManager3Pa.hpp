//
// Created by Yifei Yang on 10/1/23.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3PA_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3PA_HPP

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager3.hpp"

namespace fpdb::store::server::flight {

/**
 * Pushdown amenability-aware version of AdaptPushdownManager3
 */
class AdaptPushdownManager3Pa: public AdaptPushdownManager3 {
  friend class AdaptPushdownManager3;

public:
  AdaptPushdownManager3Pa() = default;
  ~AdaptPushdownManager3Pa() override = default;

private:
  // compute PA value
  void computePa(const std::shared_ptr<AdaptPushdownReqInfo3> &req, int64_t pullupTime, int64_t pushdownTime);

  // manage the wait queue, PA-aware impl
  void enqueue(const std::shared_ptr<AdaptPushdownReqInfo3> &req) override;
  std::shared_ptr<AdaptPushdownReqInfo3> dequeue(bool isPushedBack, bool isDoubleExecReqQueue = false) override;
};

}

#endif //FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_ADAPTIVE_ADAPTPUSHDOWNMANAGER3PA_HPP

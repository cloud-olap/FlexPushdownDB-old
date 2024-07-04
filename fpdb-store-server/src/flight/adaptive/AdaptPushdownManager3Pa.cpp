//
// Created by Yifei Yang on 10/1/23.
//

#include "fpdb/store/server/flight/adaptive/AdaptPushdownManager3Pa.hpp"
#include "algorithm"

namespace fpdb::store::server::flight {

void AdaptPushdownManager3Pa::computePa(const std::shared_ptr<AdaptPushdownReqInfo3> &req,
                                        int64_t pullupTime, int64_t pushdownTime) {
  req->extraInfo_.pa_ = pullupTime - pushdownTime;
}

void AdaptPushdownManager3Pa::enqueue(const std::shared_ptr<AdaptPushdownReqInfo3> &req) {
  if (!req->extraInfo_.isDoubleExec_) {
    auto pos = std::lower_bound(reqQueue_.begin(), reqQueue_.end(), req,
                                [](const std::shared_ptr<AdaptPushdownReqInfo3> &r1,
                                   const std::shared_ptr<AdaptPushdownReqInfo3> &r2) {
                                  return r1->extraInfo_.pa_ < r2->extraInfo_.pa_;
                                });
    reqQueue_.insert(pos, req);
  } else {
    doubleExecReqQueue_.push_back(req);
  }
}

std::shared_ptr<AdaptPushdownReqInfo3>
AdaptPushdownManager3Pa::dequeue(bool isPushedBack, bool isDoubleExecReqQueue) {
  if (!isDoubleExecReqQueue) {
    std::shared_ptr<AdaptPushdownReqInfo3> nextReq;
    if (isPushedBack) {
      nextReq = reqQueue_.front();
      reqQueue_.pop_front();
    } else {
      nextReq = reqQueue_.back();
      reqQueue_.pop_back();
    }
    nextReq->isPushedBack_ = isPushedBack;
    return nextReq;
  } else {
    return AdaptPushdownManager3::dequeue(isPushedBack, isDoubleExecReqQueue);
  }
}

}

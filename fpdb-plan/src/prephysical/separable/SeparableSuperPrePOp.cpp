//
// Created by Yifei Yang on 2/26/22.
//

#include <fpdb/plan/prephysical/separable/SeparableSuperPrePOp.h>
#include <queue>

namespace fpdb::plan::prephysical::separable {

SeparableSuperPrePOp::SeparableSuperPrePOp(uint id, const std::shared_ptr<PrePhysicalOp> &rootOp):
  PrePhysicalOp(id, SEPARABLE_SUPER),
  rootOp_(rootOp) {}

const std::shared_ptr<PrePhysicalOp> &SeparableSuperPrePOp::getRootOp() const {
  return rootOp_;
}

std::string SeparableSuperPrePOp::getTypeString() {
  return "SeparableSuperPrePOp";
}

/**
 * By definition, it will have no producers so the result of this function will never be used during trim.
 * "Used columns" here refer to used columns of all beginning operators (i.e. FilterableScanPrePOp).
 */
std::set<std::string> SeparableSuperPrePOp::getUsedColumnNames() {
  // get all beginning ops
  std::vector<std::shared_ptr<PrePhysicalOp>> beginOps;
  std::queue<std::shared_ptr<PrePhysicalOp>> opQueue;
  opQueue.push(rootOp_);
  while (!opQueue.empty()) {
    auto op = opQueue.front();
    auto producers = op->getProducers();
    if (producers.empty()) {
      beginOps.emplace_back(op);
    } else {
      for (const auto &producer: op->getProducers()) {
        opQueue.push(producer);
      }
    }
    opQueue.pop();
  }

  // collect used columns of all beginning ops
  std::set<std::string> allUsedColumnNames;
  for (const auto &beginOp: beginOps) {
    auto usedColumnNames = beginOp->getUsedColumnNames();
    allUsedColumnNames.insert(usedColumnNames.begin(), usedColumnNames.end());
  }

  return allUsedColumnNames;
}

}

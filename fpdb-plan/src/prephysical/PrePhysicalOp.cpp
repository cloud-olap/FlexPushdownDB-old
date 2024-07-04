//
// Created by Yifei Yang on 10/31/21.
//

#include <fpdb/plan/prephysical/PrePhysicalOp.h>

namespace fpdb::plan::prephysical {

PrePhysicalOp::PrePhysicalOp(uint id, PrePOpType type) :
  id_(id),
  type_(type) {}

uint PrePhysicalOp::getId() const {
  return id_;
}

PrePOpType PrePhysicalOp::getType() const {
  return type_;
}

const vector<shared_ptr<PrePhysicalOp>> &PrePhysicalOp::getProducers() const {
  return producers_;
}

const set<string> &PrePhysicalOp::getProjectColumnNames() const {
  return projectColumnNames_;
}

void PrePhysicalOp::setProducers(const vector<shared_ptr<PrePhysicalOp>> &producers) {
  producers_ = producers;
}

void PrePhysicalOp::setProjectColumnNames(const set<string> &projectColumnNames) {
  projectColumnNames_ = projectColumnNames;
}

}

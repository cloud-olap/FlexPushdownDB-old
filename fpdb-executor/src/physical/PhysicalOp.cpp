//
// Created by matt on 5/12/19.
//

#include <fpdb/executor/physical/PhysicalOp.h>
#include <spdlog/spdlog.h>
#include <cassert>               // for assert
#include <utility>               // for move

namespace fpdb::executor::physical {

PhysicalOp::PhysicalOp(std::string name,
                       POpType type,
                       std::vector<std::string> projectColumnNames,
                       int nodeId) :
  name_(std::move(name)),
  type_(type),
  projectColumnNames_(std::move(projectColumnNames)),
  nodeId_(nodeId),
  isSeparated_(false) {}

POpType PhysicalOp::getType() const {
  return type_;
}

std::string &PhysicalOp::name() {
  return name_;
}

const std::vector<std::string> &PhysicalOp::getProjectColumnNames() const {
  return projectColumnNames_;
}

int PhysicalOp::getNodeId() const {
  return nodeId_;
}

void PhysicalOp::setProducers(const std::set<std::string> &producers) {
  producers_ = producers;
}

void PhysicalOp::setConsumers(const std::set<std::string> &consumers) {
  consumers_ = consumers;
}

void PhysicalOp::produce(const std::shared_ptr<PhysicalOp> &op) {
  consumers_.emplace(op->name());
}

void PhysicalOp::consume(const std::shared_ptr<PhysicalOp> &op) {
  producers_.emplace(op->name());
}

void PhysicalOp::unProduce(const std::shared_ptr<PhysicalOp> &op) {
  consumers_.erase(op->name());
}

void PhysicalOp::unConsume(const std::shared_ptr<PhysicalOp> &op) {
  producers_.erase(op->name());
}

void PhysicalOp::reProduce(const std::string &oldOp, const std::string &newOp) {
  consumers_.erase(oldOp);
  consumers_.emplace(newOp);
}

void PhysicalOp::reConsume(const std::string &oldOp, const std::string &newOp) {
  producers_.erase(oldOp);
  producers_.emplace(newOp);
}

void PhysicalOp::clearProducers() {
  producers_.clear();
}

void PhysicalOp::clearConsumers() {
  consumers_.clear();
}

void PhysicalOp::clearConnections() {
  producers_.clear();
  consumers_.clear();
}

void PhysicalOp::addConsumerToBloomFilterInfo(const std::string &consumer,
                                              const std::string &bloomFilterCreatePOp,
                                              const std::vector<std::string> &columnNames) {
  consumerToBloomFilterInfo_[consumer] =
          std::make_shared<fpdb_store::FPDBStoreBloomFilterUseInfo>(bloomFilterCreatePOp, columnNames);
}

void PhysicalOp::setConsumerToBloomFilterInfo(const std::unordered_map<std::string,
        std::shared_ptr<fpdb_store::FPDBStoreBloomFilterUseInfo>> &consumerToBloomFilterInfo) {
  consumerToBloomFilterInfo_ = consumerToBloomFilterInfo;
}

std::set<std::string> PhysicalOp::consumers() {
  return consumers_;
}

std::set<std::string> PhysicalOp::producers() {
  return producers_;
}

const std::unordered_map<std::string, std::shared_ptr<fpdb_store::FPDBStoreBloomFilterUseInfo>>&
PhysicalOp::getConsumerToBloomFilterInfo() const {
  return consumerToBloomFilterInfo_;
}

std::shared_ptr<POpContext> PhysicalOp::ctx() {
  return opContext_;
}

void PhysicalOp::create(const std::shared_ptr<POpContext>& ctx) {
  assert (ctx);
  SPDLOG_DEBUG("Creating operator  |  name: '{}'", this->name_);
  opContext_ = ctx;
}

void PhysicalOp::setName(const std::string &Name) {
  name_ = Name;
}

void PhysicalOp::setProjectColumnNames(const std::vector<std::string> &projectColumnNames) {
  projectColumnNames_ = projectColumnNames;
}

void PhysicalOp::destroyActor() {
  opContext_->destroyActorHandles();
}

long PhysicalOp::getQueryId() const {
  return queryId_;
}

void PhysicalOp::setQueryId(long queryId) {
  queryId_ = queryId;
}

bool PhysicalOp::isSeparated() const {
  return isSeparated_;
}

void PhysicalOp::setSeparated(bool isSeparated) {
  isSeparated_ = isSeparated;
}

} // namespace


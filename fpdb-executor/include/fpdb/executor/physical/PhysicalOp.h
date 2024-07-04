//
// Created by matt on 5/12/19.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_PHYSICALOP_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_PHYSICALOP_H

#include <fpdb/executor/physical/Forward.h>
#include <fpdb/executor/physical/POpContext.h>
#include <fpdb/executor/physical/POpType.h>
#include <fpdb/executor/message/Message.h>
#include <fpdb/executor/message/Envelope.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreBloomFilterInfo.h>
#include <caf/all.hpp>
#include <string>
#include <memory>
#include <map>

namespace fpdb::executor::physical {

/**
 * Base class for physical operators
 */
class PhysicalOp {

public:
  explicit PhysicalOp(std::string name,
                      POpType type,
                      std::vector<std::string> projectColumnNames,
                      int nodeId);
  PhysicalOp() = default;
  PhysicalOp(const PhysicalOp&) = default;
  PhysicalOp& operator=(const PhysicalOp&) = default;
  virtual ~PhysicalOp() = default;

  // getters
  std::string &name();
  POpType getType() const;
  virtual std::string getTypeString() const = 0;
  const std::vector<std::string> &getProjectColumnNames() const;
  int getNodeId() const;
  long getQueryId() const;
  std::set<std::string> producers();
  std::set<std::string> consumers();
  const std::unordered_map<std::string, std::shared_ptr<fpdb_store::FPDBStoreBloomFilterUseInfo>>&
          getConsumerToBloomFilterInfo() const;
  std::shared_ptr<POpContext> ctx();

  // setters
  void setName(const std::string &Name);
  void setProjectColumnNames(const std::vector<std::string> &projectColumnNames);
  void setQueryId(long queryId);
  void setProducers(const std::set<std::string> &producers);
  void setConsumers(const std::set<std::string> &consumers);
  virtual void produce(const std::shared_ptr<PhysicalOp> &op);
  virtual void consume(const std::shared_ptr<PhysicalOp> &op);
  virtual void unProduce(const std::shared_ptr<PhysicalOp> &op);
  virtual void unConsume(const std::shared_ptr<PhysicalOp> &op);
  virtual void reProduce(const std::string &oldOp, const std::string &newOp);
  virtual void reConsume(const std::string &oldOp, const std::string &newOp);
  virtual void clearProducers();
  virtual void clearConsumers();
  void clearConnections();
  void addConsumerToBloomFilterInfo(const std::string &consumer,
                                    const std::string &bloomFilterCreatePOp,
                                    const std::vector<std::string> &columnNames);
  void setConsumerToBloomFilterInfo(const std::unordered_map<std::string,
          std::shared_ptr<fpdb_store::FPDBStoreBloomFilterUseInfo>> &consumerToBloomFilterInfo);
  void create(const std::shared_ptr<POpContext>& ctx);
  bool isSeparated() const;
  void setSeparated(bool isSeparated);

  virtual void onReceive(const fpdb::executor::message::Envelope &msg) = 0;
  virtual void clear() = 0;
  void destroyActor();

protected:
  std::string name_;
  POpType type_;
  std::vector<std::string> projectColumnNames_;
  int nodeId_;
  long queryId_;
  std::shared_ptr<POpContext> opContext_;
  std::set<std::string> producers_;
  std::set<std::string> consumers_;

  // bloom filter pushdown is embedded into the producer of BloomFilterUsePOp at storage side,
  // because we don't create BloomFilterUsePOp at storage side
  std::unordered_map<std::string, std::shared_ptr<fpdb_store::FPDBStoreBloomFilterUseInfo>> consumerToBloomFilterInfo_;

  // whether this operator is used in hybrid execution
  bool isSeparated_;

};

} // namespace

#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_PHYSICALOP_H

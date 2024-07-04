//
// Created by Yifei Yang on 4/27/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWPOP_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWPOP_H

#include <fpdb/executor/physical/PhysicalOp.h>
#include <fpdb/executor/physical/join/hashjoin/HashJoinArrowKernel.h>

namespace fpdb::executor::physical::join {

/**
 * Physical operator that does entire hash-join on top of arrow's exec kernel
 */
class HashJoinArrowPOp: public PhysicalOp {

public:
  HashJoinArrowPOp(const std::string &name,
                   const std::vector<std::string> &projectColumnNames,
                   int nodeId,
                   const HashJoinPredicate &pred,
                   JoinType joinType);
  HashJoinArrowPOp() = default;
  HashJoinArrowPOp(const HashJoinArrowPOp&) = default;
  HashJoinArrowPOp& operator=(const HashJoinArrowPOp&) = default;
  ~HashJoinArrowPOp() override = default;

  void onReceive(const Envelope &msg) override;
  void clear() override;
  void clearProducers() override;
  std::string getTypeString() const override;

  const HashJoinArrowKernel &getKernel() const;
  const std::set<std::string> &getBuildProducers() const;
  const std::set<std::string> &getProbeProducers() const;
  void setBuildProducers(const std::set<std::string> &buildProducers);
  void setProbeProducers(const std::set<std::string> &probeProducers);
  void addBuildProducer(const std::shared_ptr<PhysicalOp> &buildProducer);
  void addProbeProducer(const std::shared_ptr<PhysicalOp> &probeProducer);
  void clearBuildProducers();
  void clearProbeProducers();

private:
  void onStart();
  void onComplete(const CompleteMessage &message);
  void onTupleSet(const TupleSetMessage &message);
  void send();
  void sendEmpty();

  std::set<std::string> buildProducers_;
  std::set<std::string> probeProducers_;

  HashJoinArrowKernel kernel_;
  bool sentResult = false;
  int numCompletedBuildProducers_ = 0;
  int numCompletedProbeProducers_ = 0;

// caf inspect
public:
  template <class Inspector>
  friend bool inspect(Inspector& f, HashJoinArrowPOp& op) {
    return f.object(op).fields(f.field("name", op.name_),
                               f.field("type", op.type_),
                               f.field("projectColumnNames", op.projectColumnNames_),
                               f.field("nodeId", op.nodeId_),
                               f.field("queryId", op.queryId_),
                               f.field("opContext", op.opContext_),
                               f.field("producers", op.producers_),
                               f.field("consumers", op.consumers_),
                               f.field("consumerToBloomFilterInfo", op.consumerToBloomFilterInfo_),
                               f.field("isSeparated", op.isSeparated_),
                               f.field("buildProducers", op.buildProducers_),
                               f.field("probeProducers", op.probeProducers_),
                               f.field("kernel", op.kernel_));
  }
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWPOP_H

//
// Created by Yifei Yang on 11/20/21.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_TRANSFORM_PREPTOPTRANSFORMER_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_TRANSFORM_PREPTOPTRANSFORMER_H

#include <fpdb/executor/physical/PhysicalPlan.h>
#include <fpdb/plan/prephysical/PrePhysicalPlan.h>
#include <fpdb/plan/prephysical/SortPrePOp.h>
#include <fpdb/plan/prephysical/LimitSortPrePOp.h>
#include <fpdb/plan/prephysical/AggregatePrePOp.h>
#include <fpdb/plan/prephysical/GroupPrePOp.h>
#include <fpdb/plan/prephysical/ProjectPrePOp.h>
#include <fpdb/plan/prephysical/FilterPrePOp.h>
#include <fpdb/plan/prephysical/HashJoinPrePOp.h>
#include <fpdb/plan/prephysical/NestedLoopJoinPrePOp.h>
#include <fpdb/plan/prephysical/FilterableScanPrePOp.h>
#include <fpdb/plan/prephysical/separable/SeparableSuperPrePOp.h>
#include <fpdb/plan/Mode.h>
#include <fpdb/catalogue/CatalogueEntry.h>
#include <fpdb/catalogue/obj-store/ObjStoreConnector.h>

using namespace fpdb::plan;
using namespace fpdb::plan::prephysical;
using namespace fpdb::plan::prephysical::separable;
using namespace fpdb::catalogue;
using namespace fpdb::catalogue::obj_store;

namespace fpdb::executor::physical {

class PrePToPTransformer {

public:
  static shared_ptr<PhysicalPlan> transform(const shared_ptr<PrePhysicalPlan> &prePhysicalPlan,
                                            const shared_ptr<CatalogueEntry> &catalogueEntry,
                                            const shared_ptr<ObjStoreConnector> &objStoreConnector,
                                            const shared_ptr<Mode> &mode,
                                            int parallelDegree,
                                            int numNodes);

private:
  PrePToPTransformer(const shared_ptr<PrePhysicalPlan> &prePhysicalPlan,
                     const shared_ptr<CatalogueEntry> &catalogueEntry,
                     const shared_ptr<ObjStoreConnector> &objStoreConnector,
                     const shared_ptr<Mode> &mode,
                     int parallelDegree,
                     int numNodes);

  /**
   * Impl of transformation
   * @return
   */
  shared_ptr<PhysicalPlan> transform();

  /**
   * Transform prephysical op to physical op in dfs style
   * @param prePOp: prephysical op
   * @return physical ops to be connect to its consumers
   */
  vector<shared_ptr<PhysicalOp>> transformDfs(const shared_ptr<PrePhysicalOp> &prePOp);

  vector<vector<shared_ptr<PhysicalOp>>> transformProducers(const shared_ptr<PrePhysicalOp> &prePOp);

  vector<shared_ptr<PhysicalOp>> transformSort(const shared_ptr<SortPrePOp> &sortPrePOp);

  vector<shared_ptr<PhysicalOp>> transformLimitSort(const shared_ptr<LimitSortPrePOp> &limitSortPrePOp);

  vector<shared_ptr<PhysicalOp>> transformAggregate(const shared_ptr<AggregatePrePOp> &aggregatePrePOp);

  vector<shared_ptr<PhysicalOp>> transformGroup(const shared_ptr<GroupPrePOp> &groupPrePOp);

  vector<shared_ptr<PhysicalOp>> transformGroupOnePhase(const shared_ptr<GroupPrePOp> &groupPrePOp);

  vector<shared_ptr<PhysicalOp>> transformGroupTwoPhase(const shared_ptr<GroupPrePOp> &groupPrePOp);

  vector<shared_ptr<PhysicalOp>> transformProject(const shared_ptr<ProjectPrePOp> &projectPrePOp);

  vector<shared_ptr<PhysicalOp>> transformFilter(const shared_ptr<FilterPrePOp> &filterPrePOp);

  vector<shared_ptr<PhysicalOp>> transformHashJoin(const shared_ptr<HashJoinPrePOp> &hashJoinPrePOp);

  vector<shared_ptr<PhysicalOp>> transformNestedLoopJoin(const shared_ptr<NestedLoopJoinPrePOp> &nestedLoopJoinPrePOp);

  vector<shared_ptr<PhysicalOp>> transformFilterableScan(const shared_ptr<FilterableScanPrePOp> &filterableScanPrePOp);

  vector<shared_ptr<PhysicalOp>> transformSeparableSuper(const shared_ptr<SeparableSuperPrePOp> &separableSuperPrePOp);

  void pushdownBloomFilter(const vector<shared_ptr<PhysicalOp>> &bloomFilterCreatePOps,
                           vector<shared_ptr<PhysicalOp>> &upRightConnPOps,
                           vector<shared_ptr<PhysicalOp>> &joinProbePOps,
                           const vector<string> &rightColumnNames,
                           bool withHashJoinPushdown);

  void pushdownBloomFilterNoHashJoinPushdown(const vector<shared_ptr<PhysicalOp>> &bloomFilterCreatePOps,
                                             vector<shared_ptr<PhysicalOp>> &upRightConnPOps,
                                             vector<shared_ptr<PhysicalOp>> &joinProbePOps,
                                             const vector<string> &rightColumnNames);

  void pushdownBloomFilterWithHashJoinPushdown(const vector<shared_ptr<PhysicalOp>> &bloomFilterCreatePOps,
                                               vector<shared_ptr<PhysicalOp>> &upRightConnPOps,
                                               const vector<string> &rightColumnNames);

  void batchLoadShuffle(const vector<shared_ptr<PhysicalOp>> &opsForShuffle,
                        const vector<shared_ptr<PhysicalOp>> &shuffleConsumers,
                        vector<shared_ptr<PhysicalOp>> &allPOps,
                        bool* isHashJoinArrowLeftConn = nullptr);

  shared_ptr<PrePhysicalPlan> prePhysicalPlan_;
  shared_ptr<CatalogueEntry> catalogueEntry_;
  shared_ptr<ObjStoreConnector> objStoreConnector_;
  shared_ptr<Mode> mode_;
  int parallelDegree_;
  int numNodes_;

  unordered_map<string, shared_ptr<PhysicalOp>> physicalOps_;
};

}

#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_TRANSFORM_PREPTOPTRANSFORMER_H

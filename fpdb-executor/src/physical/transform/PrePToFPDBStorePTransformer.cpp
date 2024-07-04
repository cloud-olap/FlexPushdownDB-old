//
// Created by Yifei Yang on 3/3/22.
//

#include <fpdb/executor/physical/transform/PrePToFPDBStorePTransformer.h>
#include <fpdb/executor/physical/transform/PrePToPTransformerUtil.h>
#include <fpdb/executor/physical/transform/StoreTransformTraits.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/physical/prune/PartitionPruner.h>
#include <fpdb/executor/physical/file/RemoteFileScanPOp.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOpUtil.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreTableCacheLoadPOp.h>
#include <fpdb/executor/physical/project/ProjectPOp.h>
#include <fpdb/executor/physical/filter/FilterPOp.h>
#include <fpdb/executor/physical/aggregate/AggregatePOp.h>
#include <fpdb/executor/physical/collate/CollatePOp.h>
#include <fpdb/executor/physical/cache/CacheLoadPOp.h>
#include <fpdb/executor/physical/collect/CollectPOp.h>
#include <fpdb/executor/physical/merge/MergePOp.h>
#include <fpdb/executor/physical/shuffle/ShufflePOp.h>
#include <fpdb/executor/physical/join/hashjoin/HashJoinArrowPOp.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterCreatePOp.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterUsePOp.h>
#include <fpdb/catalogue/obj-store/ObjStoreTable.h>

using namespace fpdb::catalogue::obj_store;

namespace fpdb::executor::physical {

PrePToFPDBStorePTransformer::PrePToFPDBStorePTransformer(const shared_ptr<SeparableSuperPrePOp> &separableSuperPrePOp,
                                                         const shared_ptr<Mode> &mode,
                                                         int numComputeNodes,
                                                         int computeParallelDegree,
                                                         int fpdbStoreParallelDegree,
                                                         const shared_ptr<FPDBStoreConnector> &fpdbStoreConnector):
  separableSuperPrePOp_(separableSuperPrePOp),
  mode_(mode),
  numComputeNodes_(numComputeNodes),
  numFPDBStoreNodes_(fpdbStoreConnector->getNumHosts()),
  computeParallelDegree_(computeParallelDegree),
  fpdbStoreParallelDegree_(fpdbStoreParallelDegree),
  fpdbStoreConnector_(fpdbStoreConnector) {}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transform(const shared_ptr<SeparableSuperPrePOp> &separableSuperPrePOp,
                                       const shared_ptr<Mode> &mode,
                                       int numComputeNodes,
                                       int computeParallelDegree,
                                       int fpdbStoreParallelDegree,
                                       const shared_ptr<FPDBStoreConnector> &fpdbStoreConnector) {
  PrePToFPDBStorePTransformer transformer(separableSuperPrePOp,
                                          mode,
                                          numComputeNodes,
                                          computeParallelDegree,
                                          fpdbStoreParallelDegree,
                                          fpdbStoreConnector);
  return transformer.transform();
} 

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>> PrePToFPDBStorePTransformer::transform() {
  // transform in DFS
  auto rootPrePOp = separableSuperPrePOp_->getRootOp();
  auto transformRes = transformDfs(rootPrePOp);
  auto connPOps = transformRes.first;
  auto allPOps = transformRes.second;

  // make reduce op if needed (e.g. when the end of separable super op is aggregate)
  std::optional<std::shared_ptr<PhysicalOp>> reducePOp = std::nullopt;

  if (connPOps.size() > 1) {
    if (rootPrePOp->getType() == PrePOpType::AGGREGATE) {
      auto aggregatePrePOp = std::static_pointer_cast<AggregatePrePOp>(rootPrePOp);
      vector<string> projectColumnNames{aggregatePrePOp->getProjectColumnNames().begin(),
                                        aggregatePrePOp->getProjectColumnNames().end()};

      // aggregate reduce functions
      vector<shared_ptr<aggregate::AggregateFunction>> aggReduceFunctions;
      for (size_t j = 0; j < aggregatePrePOp->getFunctions().size(); ++j) {
        const auto &prePFunction = aggregatePrePOp->getFunctions()[j];
        const auto &aggOutputColumnName = aggregatePrePOp->getAggOutputColumnNames()[j];
        aggReduceFunctions.emplace_back(PrePToPTransformerUtil::transformAggReduceFunction(aggOutputColumnName,
                                                                                           prePFunction));
      }

      reducePOp = make_shared<aggregate::AggregatePOp>(fmt::format("Aggregate[{}]-Reduce", aggregatePrePOp->getId()),
                                                       projectColumnNames,
                                                       0,
                                                       aggReduceFunctions);
    }
  }

  // according to the mode
  switch (mode_->id()) {
    case PULL_UP:
    case CACHING_ONLY: {
      // connect operators as usual, check whether has reduce op
      if (!reducePOp.has_value()) {
        return transformRes;
      } else {
        PrePToPTransformerUtil::connectManyToOne(connPOps, *reducePOp);
        allPOps.emplace_back(*reducePOp);
        return make_pair(vector<shared_ptr<PhysicalOp>>{*reducePOp}, allPOps);
      }
    }
    case PUSHDOWN_ONLY:
    case HYBRID: {
      if (hashJoinTransInfo_.enabled_) {
        // if hash join pushdown is enabled
        // make FPDB store super ops (a single big one for each store node)
        vector<unordered_map<string, shared_ptr<PhysicalOp>>> opMaps{numFPDBStoreNodes_};
        vector<vector<shared_ptr<PhysicalOp>>> connPOpsPerStoreNode{numFPDBStoreNodes_};
        vector<string> collatePOps;
        for (const auto &op: allPOps) {
          opMaps[hashJoinTransInfo_.opToStoreNode_[op->name()]].emplace(op->name(), op);
        }
        for (const auto &op: connPOps) {
          connPOpsPerStoreNode[hashJoinTransInfo_.opToStoreNode_[op->name()]].emplace_back(op);
        }
        for (uint i = 0; i < numFPDBStoreNodes_; ++i) {
          shared_ptr<PhysicalOp> collatePOp = make_shared<collate::CollatePOp>(
                  fmt::format("Collate[{}]-node-{}", rootPrePOp->getId(), i),
                  vector<string>{},  // never used
                  0);                // never used
          collatePOps.emplace_back(collatePOp->name());
          opMaps[i][collatePOp->name()] = collatePOp;
          PrePToPTransformerUtil::connectManyToOne(connPOpsPerStoreNode[i], collatePOp);
        }
        vector<shared_ptr<PhysicalOp>> fpdbStoreSuperPOps;
        for (uint i = 0; i < numFPDBStoreNodes_; ++i) {
          auto fpdbStoreSuperPOp = make_shared<fpdb_store::FPDBStoreSuperPOp>(
                  fmt::format("FPDBStoreSuper[{}]-node-{}", rootPrePOp->getId(), i),
                  vector<string>{},  // never used
                  0,                 // never used
                  make_shared<PhysicalPlan>(opMaps[i], collatePOps[i]),
                  fpdbStoreParallelDegree_,
                  fpdbStoreConnector_->getHost(i),
                  fpdbStoreConnector_->getFileServicePort(),
                  fpdbStoreConnector_->getFlightPort());
          fpdbStoreSuperPOp->setReceiveByOthers(true);
          fpdbStoreSuperPOps.emplace_back(fpdbStoreSuperPOp);
        }

        // make FPDBStoreTableCacheLoadPOps to fetch results
        vector<shared_ptr<PhysicalOp>> fpdbStoreTableCacheLoadPOps;
        for (int i = 0; i < (int) numFPDBStoreNodes_; ++i) {
          auto fpdbStoreSuperPOp = fpdbStoreSuperPOps[i];
          auto typedFPDBStoreSuperPOp = static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(fpdbStoreSuperPOp);
          vector<shared_ptr<PhysicalOp>> fpdbStoreTableCacheLoadPOpsPerNode;
          for (int j = 0; j < fpdbStoreParallelDegree_; ++j) {
            fpdbStoreTableCacheLoadPOpsPerNode.emplace_back(make_shared<fpdb_store::FPDBStoreTableCacheLoadPOp>(
                    fmt::format("FPDBStoreTableCacheLoad[{}]-{}", rootPrePOp->getId(), i*fpdbStoreParallelDegree_ + j),
                    vector<string>{},   // never used
                    i % numComputeNodes_));
          }
          fpdbStoreTableCacheLoadPOps.insert(fpdbStoreTableCacheLoadPOps.end(),
                                             fpdbStoreTableCacheLoadPOpsPerNode.begin(),
                                             fpdbStoreTableCacheLoadPOpsPerNode.end());
          PrePToPTransformerUtil::connectOneToMany(fpdbStoreSuperPOp, fpdbStoreTableCacheLoadPOpsPerNode);
          typedFPDBStoreSuperPOp->setForwardConsumers(fpdbStoreTableCacheLoadPOpsPerNode);
        }

        connPOps = fpdbStoreTableCacheLoadPOps;
        allPOps = fpdbStoreSuperPOps;
        allPOps.insert(allPOps.end(), fpdbStoreTableCacheLoadPOps.begin(), fpdbStoreTableCacheLoadPOps.end());
      } else {
        // if hash join pushdown is not enabled
        // add collate at the end of each FPDB store super op
        vector<shared_ptr<PhysicalOp>> collatePOps;
        for (size_t i = 0; i < connPOps.size(); ++i) {
          collatePOps.emplace_back(
                  make_shared<collate::CollatePOp>(fmt::format("Collate[{}]-{}", rootPrePOp->getId(), i),
                                                   connPOps[i]->getProjectColumnNames(),
                                                   connPOps[i]->getNodeId()));
        }
        PrePToPTransformerUtil::connectOneToOne(connPOps, collatePOps);

        // make FPDB store super ops
        unordered_map<string, shared_ptr<PhysicalOp>> opMap;
        for (const auto &op: allPOps) {
          opMap.emplace(op->name(), op);
        }
        vector<shared_ptr<PhysicalOp>> fpdbStoreSuperPOps;
        for (uint i = 0; i < collatePOps.size(); ++i) {
          auto rootOpToPlanAndHostRes = PrePToPTransformerUtil::rootOpToPlanAndHost(collatePOps[i], opMap,
                                                                                    objectToHost_);
          const auto &subPlan = rootOpToPlanAndHostRes.first;
          const auto &host = rootOpToPlanAndHostRes.second;
          fpdbStoreSuperPOps.emplace_back(make_shared<fpdb_store::FPDBStoreSuperPOp>(
                  fmt::format("FPDBStoreSuper[{}]-{}", rootPrePOp->getId(), i),
                  collatePOps[i]->getProjectColumnNames(),
                  collatePOps[i]->getNodeId(),
                  subPlan,
                  1,
                  host,
                  fpdbStoreConnector_->getFileServicePort(),
                  fpdbStoreConnector_->getFlightPort()));
        }

        // transform to hybrid execution plan, if needed
        if (mode_->id() == HYBRID) {
          auto hybridTransformRes = transformPushdownOnlyToHybrid(fpdbStoreSuperPOps);
          connPOps = hybridTransformRes.first;
          allPOps = hybridTransformRes.second;
        } else {
          connPOps = fpdbStoreSuperPOps;
          allPOps = fpdbStoreSuperPOps;
        }
      }

      // check whether has reduce op
      if (!reducePOp.has_value()) {
        return make_pair(connPOps, allPOps);
      } else {
        PrePToPTransformerUtil::connectManyToOne(connPOps, *reducePOp);
        allPOps.emplace_back(*reducePOp);
        return make_pair(vector<shared_ptr<PhysicalOp>>{*reducePOp}, allPOps);
      }
    }
    default:
      throw runtime_error(fmt::format("Unsupported mode for FPDB Store: {}", mode_->toString()));
  }
}

std::tuple<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>, bool>
PrePToFPDBStorePTransformer::addSeparablePOp(vector<shared_ptr<PhysicalOp>> &producers,
                                             vector<shared_ptr<PhysicalOp>> &separablePOps,
                                             const shared_ptr<Mode> &mode,
                                             unordered_map<string, shared_ptr<PhysicalOp>>* opMap,
                                             int prePOpId,
                                             int numComputeNodes,
                                             int computeParallelDegree) {
  // check mode
  if (mode->id() == ModeId::PULL_UP || mode->id() == ModeId::CACHING_ONLY) {
    PrePToPTransformerUtil::connectOneToOne(producers, separablePOps);
    return {separablePOps, separablePOps, false};
  }
  // TODO: support hybrid
  if (mode->id() != ModeId::PUSHDOWN_ONLY) {
    PrePToPTransformerUtil::connectOneToOne(producers, separablePOps);
    return {separablePOps, separablePOps, false};
  }

  // check if hash-join is pushed
  // FIXME: not a good way to check by whether the producer is FPDBStoreTableCacheLoadPOp
  bool withHashJoinPushdown = false;
  for (const auto &producer: producers) {
    if (producer->getType() == POpType::FPDB_STORE_TABLE_CACHE_LOAD) {
      withHashJoinPushdown = true;
      break;
    }
  }

  // do accordingly
  if (withHashJoinPushdown) {
    const auto &res = addSeparablePOpWithHashJoinPushdown(producers, separablePOps,
                                                          opMap, prePOpId, numComputeNodes, computeParallelDegree);
    return {res.first, res.second, true};
  } else {
    const auto &res = addSeparablePOpNoHashJoinPushdown(producers, separablePOps);
    return {res.first, res.second, false};
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::addSeparablePOpNoHashJoinPushdown(vector<shared_ptr<PhysicalOp>> &producers,
                                                               vector<shared_ptr<PhysicalOp>> &separablePOps) {

  vector<shared_ptr<PhysicalOp>> connPOps, addiPOps;

  for (uint i = 0; i < producers.size(); ++i) {
    auto producer = producers[i];
    auto separablePOp = separablePOps[i];

    // pushable when the operator type is enabled for pushdown, and the producer is FPDBStoreSuperPOp
    if (StoreTransformTraits::FPDBStoreStoreTransformTraits()->isSeparable(separablePOp->getType()) &&
        producer->getType() == POpType::FPDB_STORE_SUPER) {
      separablePOp->setSeparated(true);
      auto fpdbStoreSuperPOp = static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(producer);
      auto res = fpdbStoreSuperPOp->getSubPlan()->addAsLast(separablePOp);
      if (!res.has_value()) {
        throw runtime_error(res.error());
      }
      connPOps.emplace_back(fpdbStoreSuperPOp);

      // need to handle shuffle op specially (remove collatePOp from its consumeVec)
      if (separablePOp->getType() == POpType::SHUFFLE) {
        fpdbStoreSuperPOp->setShufflePOp(separablePOp);
        std::static_pointer_cast<shuffle::ShufflePOp>(separablePOp)->clearConsumerVec();
      }

      // need to handle group op specially
      // (set projectColumnNames for both fpdbStoreSuperPOp and root op of the subPlan)
      else if (separablePOp->getType() == POpType::GROUP) {
        auto expRootOp = fpdbStoreSuperPOp->getSubPlan()->getRootPOp();
        if (!expRootOp.has_value()) {
          throw runtime_error(expRootOp.error());
        }
        (*expRootOp)->setProjectColumnNames(separablePOp->getProjectColumnNames());
        fpdbStoreSuperPOp->setProjectColumnNames(separablePOp->getProjectColumnNames());
      }
    } else {
      PrePToPTransformerUtil::connectOneToOne(producer, separablePOp);
      connPOps.emplace_back(separablePOp);
      addiPOps.emplace_back(separablePOp);
    }
  }

  return {connPOps, addiPOps};
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::addSeparablePOpWithHashJoinPushdown(vector<shared_ptr<PhysicalOp>> &producers,
                                                                 vector<shared_ptr<PhysicalOp>> &separablePOps,
                                                                 unordered_map<string, shared_ptr<PhysicalOp>>* opMap,
                                                                 int prePOpId,
                                                                 int numComputeNodes,
                                                                 int computeParallelDegree) {
  // get all FPDBStoreSuperPOps in order
  set<string> fpdbStoreSuperPOpNameSet;
  vector<string> fpdbStoreSuperPOpNames;
  for (const auto &producer: producers) {
    if (producer->getType() == POpType::FPDB_STORE_TABLE_CACHE_LOAD) {
      auto fpdbStoreSuperPOpName = static_pointer_cast<fpdb_store::FPDBStoreTableCacheLoadPOp>(producer)->getProducer();
      if (fpdbStoreSuperPOpNameSet.find(fpdbStoreSuperPOpName) == fpdbStoreSuperPOpNameSet.end()) {
        fpdbStoreSuperPOpNames.emplace_back(fpdbStoreSuperPOpName);
        fpdbStoreSuperPOpNameSet.emplace(fpdbStoreSuperPOpName);
      }
    }
  }

  // check if the operator type is enabled for pushdown
  if (!StoreTransformTraits::FPDBStoreStoreTransformTraits()->isSeparable(separablePOps[0]->getType())) {
    PrePToPTransformerUtil::connectOneToOne(producers, separablePOps);
    return {separablePOps, separablePOps};
  }

  // add separablePOps to the subPlan of FPDBStoreSuperPOps, need to trait shuffle specially
  if (separablePOps[0]->getType() == POpType::SHUFFLE) {
    // remove "producers" from opMap because shuffle needs different number of FPDBStoreTableCacheLoadPOps
    for (const auto &producer: producers) {
      opMap->erase(producer->name());
    }

    // CollectPOps to combine tables from multiple FPDBStoreTableCacheLoadPOps that belong to the same shuffle piece
    vector<shared_ptr<PhysicalOp>> collectPOps;
    for (int i = 0; i < computeParallelDegree * numComputeNodes; ++i) {
      collectPOps.emplace_back(make_shared<collect::CollectPOp>(fmt::format("Collect[{}]-{}", prePOpId, i),
                                                                vector<string>{},  // never used
                                                                i % numComputeNodes));
    }

    // new FPDBStoreTableCacheLoadPOps and add separablePOPs to FPDBStoreSuperPOps,
    vector<shared_ptr<PhysicalOp>> newFPDBStoreTableCacheLoadPOps;
    uint separablePOpsOffset = 0;
    for (uint i = 0; i < fpdbStoreSuperPOpNames.size(); ++i) {
      auto fpdbStoreSuperPOp = opMap->find(fpdbStoreSuperPOpNames[i])->second;
      // clear consumers of fpdbStoreSuperPOp first
      fpdbStoreSuperPOp->clearConsumers();
      // make new FPDBStoreTableCacheLoadPOps
      vector<string> newFPDBStoreTableCacheLoadPOpNamesPerStoreNode;
      for (int j = 0; j < computeParallelDegree * numComputeNodes; ++j) {
        shared_ptr<PhysicalOp> newFPDBStoreTableCacheLoadPOp = make_shared<fpdb_store::FPDBStoreTableCacheLoadPOp>(
                fmt::format("FPDBStoreTableCacheLoad[{}]-node-{}-{}", prePOpId, i, j),
                vector<string>{},   // never used
                j % numComputeNodes);
        newFPDBStoreTableCacheLoadPOps.emplace_back(newFPDBStoreTableCacheLoadPOp);
        newFPDBStoreTableCacheLoadPOpNamesPerStoreNode.emplace_back(newFPDBStoreTableCacheLoadPOp->name());
        // connect FPDBStoreSuperPOp to this
        PrePToPTransformerUtil::connectOneToOne(fpdbStoreSuperPOp, newFPDBStoreTableCacheLoadPOp);
        // connect this to CollectPOp
        PrePToPTransformerUtil::connectOneToOne(newFPDBStoreTableCacheLoadPOp, collectPOps[j]);
      }

      // add separablePOps to FPDBStoreSuperPOps
      auto typedFPDBStoreSuperPOp = static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(fpdbStoreSuperPOp);
      auto expRootPOp = typedFPDBStoreSuperPOp->getSubPlan()->getRootPOp();
      if (!expRootPOp.has_value()) {
        throw runtime_error(expRootPOp.error());
      }
      static_pointer_cast<collate::CollatePOp>(*expRootPOp)
              ->setEndConsumers(newFPDBStoreTableCacheLoadPOpNamesPerStoreNode);
      auto rootNumProducers = (*expRootPOp)->producers().size();
      vector<shared_ptr<PhysicalOp>> subOps = {separablePOps.begin() + separablePOpsOffset,
                                               separablePOps.begin() + separablePOpsOffset + rootNumProducers};
      typedFPDBStoreSuperPOp->getSubPlan()->addAsLasts(subOps);
      for (const auto &shufflePOp: subOps) {
        shufflePOp->setSeparated(true);
        std::static_pointer_cast<shuffle::ShufflePOp>(shufflePOp)
                ->setConsumerVec(newFPDBStoreTableCacheLoadPOpNamesPerStoreNode);
      }
      separablePOpsOffset += rootNumProducers;
    }
    auto addiPOps = newFPDBStoreTableCacheLoadPOps;
    addiPOps.insert(addiPOps.end(), collectPOps.begin(), collectPOps.end());
    return {collectPOps, addiPOps};
  } else {
    // regular case (when separablePOps are not shuffle)
    uint separablePOpsOffset = 0;
    for (const auto &fpdbStoreSuperPOpName: fpdbStoreSuperPOpNames) {
      auto fpdbStoreSuperPOp =
              static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(opMap->find(fpdbStoreSuperPOpName)->second);
      auto expRootPOp = fpdbStoreSuperPOp->getSubPlan()->getRootPOp();
      if (!expRootPOp.has_value()) {
        throw runtime_error(expRootPOp.error());
      }
      auto rootNumProducers = (*expRootPOp)->producers().size();
      vector<shared_ptr<PhysicalOp>> subOps = {separablePOps.begin() + separablePOpsOffset,
                                               separablePOps.begin() + separablePOpsOffset + rootNumProducers};
      fpdbStoreSuperPOp->getSubPlan()->addAsLasts(subOps);
      fpdbStoreSuperPOp->resetForwardConsumers();
      separablePOpsOffset += rootNumProducers;
    }
    return {producers, {}};
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformDfs(const shared_ptr<PrePhysicalOp> &prePOp) {
  switch (prePOp->getType()) {
    case PrePOpType::FILTERABLE_SCAN: {
      const auto &filterableScanPrePOp = std::static_pointer_cast<FilterableScanPrePOp>(prePOp);
      return transformFilterableScan(filterableScanPrePOp);
    }
    case PrePOpType::PROJECT: {
      const auto &projectPrePOp = std::static_pointer_cast<ProjectPrePOp>(prePOp);
      return transformProject(projectPrePOp);
    }
    case PrePOpType::FILTER: {
      const auto &filterPrePOp = std::static_pointer_cast<FilterPrePOp>(prePOp);
      return transformFilter(filterPrePOp);
    }
    case PrePOpType::AGGREGATE: {
      const auto &aggregatePrePOp = std::static_pointer_cast<AggregatePrePOp>(prePOp);
      return transformAggregate(aggregatePrePOp);
    }
    case PrePOpType::HASH_JOIN: {
      const auto &hashJoinPrePOp = std::static_pointer_cast<HashJoinPrePOp>(prePOp);
      return transformHashJoin(hashJoinPrePOp);
    }
    default: {
      throw runtime_error(fmt::format("Unsupported prephysical operator type for FPDB store: {}", prePOp->getTypeString()));
    }
  }
}

vector<pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>>
PrePToFPDBStorePTransformer::transformProducers(const shared_ptr<PrePhysicalOp> &prePOp) {
  vector<pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>> transformRes;
  for (const auto &producer: prePOp->getProducers()) {
    transformRes.emplace_back(transformDfs(producer));
  }
  return transformRes;
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformPushdownOnlyToHybrid(const vector<shared_ptr<PhysicalOp>> &fpdbStoreSuperPOps) {
  vector<shared_ptr<PhysicalOp>> connPOps, allPOps;
  if (fpdbStoreSuperPOps.empty()) {
    return {connPOps, allPOps};
  }

  // get projectColumnGroups needed for hybrid execution, this is same for all fpdbStoreSuperPOps
  auto projectColumnGroups = fpdb_store::FPDBStoreSuperPOpUtil::getProjectColumnGroups(
          static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(fpdbStoreSuperPOps[0]));

  for (const auto &fpdbStoreSuperPOp: fpdbStoreSuperPOps) {
    auto typedFPDBStoreSuperPOp = static_pointer_cast<fpdb_store::FPDBStoreSuperPOp>(fpdbStoreSuperPOp);

    // set waitForScanMessage, so it won't run until receiving scan message
    typedFPDBStoreSuperPOp->setWaitForScanMessage(true);

    unordered_map<shared_ptr<PhysicalOp>, shared_ptr<PhysicalOp>> storePOpToLocalPOp;
    unordered_map<string, shared_ptr<PhysicalOp>> storePOpMap = typedFPDBStoreSuperPOp->getSubPlan()->getPhysicalOps();
    unordered_map<string, shared_ptr<PhysicalOp>> localPOpMap;
    unordered_map<string, string> storePOpRenames;
    optional<shared_ptr<merge::MergePOp>> mergePOp1, mergePOp2;

    // get predicateColumnNames needed for hybrid execution, this may be different for some fpdbStoreSuperPOps
    // so we need to get it for each one
    auto predicateColumnNames = fpdb_store::FPDBStoreSuperPOpUtil::getPredicateColumnNames(typedFPDBStoreSuperPOp);

    // create mirror ops which are executed locally
    for (const auto &storePOpIt: storePOpMap) {
      auto storePOp = storePOpIt.second;
      if (storePOp->getType() == POpType::COLLATE) {
        continue;
      }

      shared_ptr<PhysicalOp> localPOp;
      bool isMirrored = false;
      switch (storePOp->getType()) {
        case POpType::FPDB_STORE_FILE_SCAN: {
          auto fpdbStoreFileScanPOp = static_pointer_cast<fpdb_store::FPDBStoreFileScanPOp>(storePOp);
          auto bucket = fpdbStoreFileScanPOp->getBucket();
          auto object = fpdbStoreFileScanPOp->getObject();
          auto kernel = fpdbStoreFileScanPOp->getKernel();
          auto fileSize = kernel->getFileSize();

          auto cacheLoadPOp = make_shared<cache::CacheLoadPOp>(fmt::format("CacheLoad[{}]-{}/{}",
                                                                           separableSuperPrePOp_->getId(),
                                                                           bucket,
                                                                           object),
                                                               fpdbStoreFileScanPOp->getProjectColumnNames(),
                                                               fpdbStoreFileScanPOp->getNodeId(),
                                                               predicateColumnNames,
                                                               projectColumnGroups,
                                                               fpdbStoreFileScanPOp->getProjectColumnNames(),
                                                               make_shared<ObjStorePartition>(bucket, object, fileSize),
                                                               0,
                                                               fileSize,
                                                               fpdbStoreConnector_);
          localPOp = cacheLoadPOp;

          // Create a RemoteFileScanPOp to load data to cache, and a MergePOp, then wire them up
          auto missPOpToCache = make_shared<file::RemoteFileScanPOp>(fmt::format("RemoteFileScan(cache)[{}]-{}/{}",
                                                                                 separableSuperPrePOp_->getId(),
                                                                                 bucket,
                                                                                 object),
                                                                     fpdbStoreFileScanPOp->getProjectColumnNames(),
                                                                     fpdbStoreFileScanPOp->getNodeId(),
                                                                     bucket,
                                                                     object,
                                                                     kernel->getFormat(),
                                                                     kernel->getSchema(),
                                                                     fileSize,
                                                                     typedFPDBStoreSuperPOp->getHost(),
                                                                     fpdbStoreConnector_->getFileServicePort(),
                                                                     nullopt,
                                                                     false,
                                                                     true);
          // first merge for cached data and to-cache data
          mergePOp1 = make_shared<merge::MergePOp>(fmt::format("merge1[{}]-{}/{}",
                                                               separableSuperPrePOp_->getId(),
                                                               bucket,
                                                               object),
                                                   fpdbStoreFileScanPOp->getProjectColumnNames(),
                                                   fpdbStoreFileScanPOp->getNodeId());
          cacheLoadPOp->setHitOperator(*mergePOp1);
          (*mergePOp1)->setLeftProducer(cacheLoadPOp);
          cacheLoadPOp->setMissOperatorToCache(missPOpToCache);
          missPOpToCache->consume(cacheLoadPOp);
          missPOpToCache->produce(*mergePOp1);
          (*mergePOp1)->setRightProducer(missPOpToCache);

          // miss operator to push down
          cacheLoadPOp->setMissOperatorToPushdown(fpdbStoreSuperPOp);
          fpdbStoreSuperPOp->consume(cacheLoadPOp);

          // second merge for local result and pushdown result
          mergePOp2 = make_shared<merge::MergePOp>(fmt::format("merge2[{}]-{}/{}",
                                                               separableSuperPrePOp_->getId(),
                                                               bucket,
                                                               object),
                                                   fpdbStoreSuperPOp->getProjectColumnNames(),
                                                   fpdbStoreSuperPOp->getNodeId());

          // save created ops
          connPOps.emplace_back(*mergePOp2);
          allPOps.emplace_back(missPOpToCache);
          allPOps.emplace_back(fpdbStoreSuperPOp);
          allPOps.emplace_back(*mergePOp1);
          allPOps.emplace_back(*mergePOp2);

          break;
        }
        case POpType::FILTER: {
          localPOp = make_shared<filter::FilterPOp>(dynamic_cast<filter::FilterPOp &>(*storePOp));
          isMirrored = true;
          break;
        }
        case POpType::PROJECT: {
          localPOp = make_shared<project::ProjectPOp>(dynamic_cast<project::ProjectPOp &>(*storePOp));
          isMirrored = true;
          break;
        }
        case POpType::AGGREGATE: {
          localPOp = make_shared<aggregate::AggregatePOp>(dynamic_cast<aggregate::AggregatePOp &>(*storePOp));
          isMirrored = true;
          break;
        }
        default: {
          throw runtime_error(fmt::format("Unsupported physical operator type at FPDB store: {}", storePOp->getTypeString()));
        }
      }

      // clear connections and set separated for mirrored operators
      if (isMirrored) {
        localPOp->clearConnections();
        localPOp->setName(fmt::format("{}-separated(local)", localPOp->name()));
        localPOp->setSeparated(true);
        storePOpRenames.emplace(storePOp->name(), fmt::format("{}-separated(fpdb-store)", storePOp->name()));
        storePOp->setSeparated(true);
      }

      storePOpToLocalPOp.emplace(storePOp, localPOp);
      localPOpMap.emplace(localPOp->name(), localPOp);
      allPOps.emplace_back(localPOp);
    }

    if (!mergePOp1.has_value() || !mergePOp2.has_value()) {
      throw runtime_error("FPDBStoreFileScanPOp not found in pushdown subPlan");
    }

    // wire up local ops following the connection among store ops
    for (const auto &storeToLocalIt: storePOpToLocalPOp) {
      auto storePOp = storeToLocalIt.first;
      auto localPOp = storeToLocalIt.second;

      if (localPOp->getType() == POpType::CACHE_LOAD) {
        // already completed connected when created
        continue;
      } else {
        for (const auto &storeConsumerName: storePOp->consumers()) {
          auto storeConsumer = storePOpMap.find(storeConsumerName)->second;
          // need to get rid of CollatePOp here
          if (storeConsumer->getType() == POpType::COLLATE) {
            continue;
          }
          auto localConsumer = storePOpToLocalPOp.find(storeConsumer)->second;
          localPOp->produce(localConsumer);
        }
        for (const auto &storeProducerName: storePOp->producers()) {
          auto storeProducer = storePOpMap.find(storeProducerName)->second;
          auto localProducer = storePOpToLocalPOp.find(storeProducer)->second;

          // if the corresponding local producer is CacheLoadPOp, then it needs to be connected to mergePOp1
          if (localProducer->getType() == POpType::CACHE_LOAD) {
            (*mergePOp1)->produce(localPOp);
            localPOp->consume(*mergePOp1);
          } else {
            localPOp->consume(localProducer);
          }
        }
      }
    }

    // wire up for the second merge
    auto expStoreCollatePOp = typedFPDBStoreSuperPOp->getSubPlan()->getRootPOp();
    if (!expStoreCollatePOp.has_value()) {
      throw runtime_error(expStoreCollatePOp.error());
    }
    auto storeCollatePOp = *expStoreCollatePOp;

    if (storeCollatePOp->producers().size() > 1) {
      throw runtime_error("Currently pushdown only supports plan with serial operators");
    }
    auto storeLastPOpName = *storeCollatePOp->producers().begin();
    auto storeLastPOp = storePOpMap.find(storeLastPOpName)->second;
    auto localLastPOp = storePOpToLocalPOp.find(storeLastPOp)->second;
    if (localLastPOp->getType() == POpType::CACHE_LOAD) {
      // this means the separable part is just a scan operator, so needs to set mergePOp1 as the last local op
      localLastPOp = *mergePOp1;
    }

    (*mergePOp2)->setLeftProducer(localLastPOp);
    localLastPOp->produce(*mergePOp2);
    (*mergePOp2)->setRightProducer(fpdbStoreSuperPOp);
    fpdbStoreSuperPOp->produce(*mergePOp2);

    // rename store ops
    for (const auto &renameIt: storePOpRenames) {
      typedFPDBStoreSuperPOp->getSubPlan()->renamePOp(renameIt.first, renameIt.second);
    }

    // enable bitmap pushdown if required
    if (StoreTransformTraits::FPDBStoreStoreTransformTraits()->isFilterBitmapPushdownEnabled()) {
      enableBitmapPushdown(storePOpToLocalPOp, typedFPDBStoreSuperPOp);
    }
  }

  return {connPOps, allPOps};
}

void PrePToFPDBStorePTransformer::enableBitmapPushdown(
        const unordered_map<shared_ptr<PhysicalOp>, shared_ptr<PhysicalOp>> &storePOpToLocalPOp,
        const shared_ptr<fpdb_store::FPDBStoreSuperPOp> &fpdbStoreSuperPOp) {
  for (const auto &storeToLocalIt: storePOpToLocalPOp) {
    auto storePOp = storeToLocalIt.first;
    auto localPOp = storeToLocalIt.second;

    switch (localPOp->getType()) {
      case CACHE_LOAD: {
        auto typedLocalPOp = static_pointer_cast<cache::CacheLoadPOp>(localPOp);
        typedLocalPOp->enableBitmapPushdown();
        break;
      }
      case FILTER: {
        auto typedLocalPOp = static_pointer_cast<filter::FilterPOp>(localPOp);
        auto typedStorePOp = static_pointer_cast<filter::FilterPOp>(storePOp);
        typedLocalPOp->enableBitmapPushdown(fpdbStoreSuperPOp->name(), typedStorePOp->name(), true,
                                            fpdbStoreSuperPOp->getHost(), fpdbStoreSuperPOp->getFlightPort());
        typedStorePOp->enableBitmapPushdown(fpdbStoreSuperPOp->name(), typedLocalPOp->name(), false,
                                            fpdbStoreSuperPOp->getHost(), fpdbStoreSuperPOp->getFlightPort());
        break;
      }
      default: {
        // noop
      }
    }
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformFilterableScan(const shared_ptr<FilterableScanPrePOp> &filterableScanPrePOp) {
  const auto &objStoreTable = std::static_pointer_cast<ObjStoreTable>(filterableScanPrePOp->getTable());
  const auto &partitions = (const vector<shared_ptr<Partition>> &) objStoreTable->getObjStorePartitions();
  const auto &partitionPredicates = PartitionPruner::prune(partitions, filterableScanPrePOp->getPredicate());

  switch (mode_->id()) {
    case PULL_UP:
      return transformFilterableScanPullup(filterableScanPrePOp, partitionPredicates);
    case PUSHDOWN_ONLY:
    case HYBRID:
      // note: hybrid will transform pushdown-only plan to hybrid after the entire plan is transformed
      // so here both modes call the same function
      return transformFilterableScanPushdownOnly(filterableScanPrePOp, partitionPredicates);
    case CACHING_ONLY:
      return transformFilterableScanCachingOnly(filterableScanPrePOp, partitionPredicates);
    default:
      throw runtime_error(fmt::format("Unsupported mode for FPDB Store: {}", mode_->toString()));
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformFilterableScanPullup(const shared_ptr<FilterableScanPrePOp> &filterableScanPrePOp,
                              const unordered_map<shared_ptr<Partition>, shared_ptr<Expression>, PartitionPointerHash, PartitionPointerPredicate> &partitionPredicates) {
  vector<shared_ptr<PhysicalOp>> scanPOps, filterPOps;
  const auto &table = filterableScanPrePOp->getTable();
  vector<string> projectColumnNames{filterableScanPrePOp->getProjectColumnNames().begin(),
                                    filterableScanPrePOp->getProjectColumnNames().end()};

  /**
   * For each partition, construct:
   * a RemoteFileScan, a Filter if needed
   */

  uint partitionId = 0;
  for (const auto &partitionPredicateIt: partitionPredicates) {
    const auto &partition = static_pointer_cast<ObjStorePartition>(partitionPredicateIt.first);
    const auto &predicate = partitionPredicateIt.second;
    const auto &bucket = partition->getBucket();
    const auto &object = partition->getObject();
    const auto &fpdbStoreNodeId = partition->getNodeId();
    if (!fpdbStoreNodeId.has_value()) {
      throw runtime_error(fmt::format("Node id not set for FPDB store object: {}", partition->getObject()));
    }

    // project column names and its union with project column names
    vector<string> predicateColumnNames;
    if (predicate) {
      const auto predicateColumnNameSet = predicate->involvedColumnNames();
      predicateColumnNames.assign(predicateColumnNameSet.begin(), predicateColumnNameSet.end());
    }
    const auto &projPredColumnNames = union_(projectColumnNames, predicateColumnNames);

    // remote file scan
    const auto &scanPOp = make_shared<file::RemoteFileScanPOp>(fmt::format("RemoteFileScan[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                               projPredColumnNames,
                                                               partitionId % numComputeNodes_,
                                                               bucket,
                                                               object,
                                                               table->getFormat(),
                                                               table->getSchema(),
                                                               partition->getNumBytes(),
                                                               fpdbStoreConnector_->getHost(*fpdbStoreNodeId),
                                                               fpdbStoreConnector_->getFileServicePort());
    scanPOps.emplace_back(scanPOp);

    // filter
    if (predicate) {
      const auto &filterPOp = make_shared<filter::FilterPOp>(fmt::format("Filter[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                             projectColumnNames,
                                                             partitionId % numComputeNodes_,
                                                             predicate,
                                                             table);
      filterPOps.emplace_back(filterPOp);
      scanPOp->produce(filterPOp);
      filterPOp->consume(scanPOp);
    }

    ++partitionId;
  }

  if (filterPOps.empty()) {
    return make_pair(scanPOps, scanPOps);
  } else {
    vector<shared_ptr<PhysicalOp>> allPOps;
    allPOps.insert(allPOps.end(), scanPOps.begin(), scanPOps.end());
    allPOps.insert(allPOps.end(), filterPOps.begin(), filterPOps.end());
    return make_pair(filterPOps, allPOps);
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformFilterableScanPushdownOnly(const shared_ptr<FilterableScanPrePOp> &filterableScanPrePOp,
                                    const unordered_map<shared_ptr<Partition>, shared_ptr<Expression>, PartitionPointerHash, PartitionPointerPredicate> &partitionPredicates) {
  vector<shared_ptr<PhysicalOp>> scanPOps, filterPOps;
  const auto &table = filterableScanPrePOp->getTable();
  vector<string> projectColumnNames{filterableScanPrePOp->getProjectColumnNames().begin(),
                                    filterableScanPrePOp->getProjectColumnNames().end()};

  /**
   * For each partition, construct:
   * a FPDBStoreFileScan, a Filter if needed
   */

  uint partitionId = 0;
  for (const auto &partitionPredicateIt: partitionPredicates) {
    const auto &partition = static_pointer_cast<ObjStorePartition>(partitionPredicateIt.first);
    const auto &predicate = partitionPredicateIt.second;
    const auto &bucket = partition->getBucket();
    const auto &object = partition->getObject();
    const auto &fpdbStoreNodeId = partition->getNodeId();
    if (!fpdbStoreNodeId.has_value()) {
      throw runtime_error(fmt::format("Node id not set for FPDB store object: {}", partition->getObject()));
    }

    // project column names and its union with project column names
    vector<string> predicateColumnNames;
    if (predicate) {
      const auto predicateColumnNameSet = predicate->involvedColumnNames();
      predicateColumnNames.assign(predicateColumnNameSet.begin(), predicateColumnNameSet.end());
    }
    const auto &projPredColumnNames = union_(projectColumnNames, predicateColumnNames);

    // FPDB Store file scan
    const auto &scanPOp = make_shared<fpdb_store::FPDBStoreFileScanPOp>(fmt::format("FPDBStoreFileScan[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                                        projPredColumnNames,
                                                                        partitionId % numComputeNodes_,
                                                                        bucket,
                                                                        object,
                                                                        table->getFormat(),
                                                                        table->getSchema(),
                                                                        partition->getNumBytes());
    scanPOps.emplace_back(scanPOp);
    objectToHost_.emplace(object, fpdbStoreConnector_->getHost(*fpdbStoreNodeId));

    // filter
    if (predicate) {
      const auto &filterPOp = make_shared<filter::FilterPOp>(fmt::format("Filter[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                             projectColumnNames,
                                                             partitionId % numComputeNodes_,
                                                             predicate,
                                                             table);
      filterPOps.emplace_back(filterPOp);
      scanPOp->produce(filterPOp);
      filterPOp->consume(scanPOp);
    }

    ++partitionId;
  }

  if (filterPOps.empty()) {
    return make_pair(scanPOps, scanPOps);
  } else {
    vector<shared_ptr<PhysicalOp>> allPOps;
    allPOps.insert(allPOps.end(), scanPOps.begin(), scanPOps.end());
    allPOps.insert(allPOps.end(), filterPOps.begin(), filterPOps.end());
    return make_pair(filterPOps, allPOps);
  }
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformFilterableScanCachingOnly(const shared_ptr<FilterableScanPrePOp> &filterableScanPrePOp,
                                      const unordered_map<shared_ptr<Partition>, shared_ptr<Expression>, PartitionPointerHash, PartitionPointerPredicate> &partitionPredicates) {
  vector<shared_ptr<PhysicalOp>> allPOps, selfConnDownPOps;
  const auto &table = filterableScanPrePOp->getTable();
  vector<string> projectColumnNames{filterableScanPrePOp->getProjectColumnNames().begin(),
                                    filterableScanPrePOp->getProjectColumnNames().end()};

  /**
   * For each partition, construct:
   * a CacheLoad, a RemoteFileScan, a Merge, a Filter if needed
   */
  uint partitionId = 0;
  for (const auto &partitionPredicateIt: partitionPredicates) {
    const auto &partition = static_pointer_cast<ObjStorePartition>(partitionPredicateIt.first);
    const auto &predicate = partitionPredicateIt.second;
    const auto &bucket = partition->getBucket();
    const auto &object = partition->getObject();
    const auto &fpdbStoreNodeId = partition->getNodeId();
    if (!fpdbStoreNodeId.has_value()) {
      throw runtime_error(fmt::format("Node id not set for FPDB store object: {}", partition->getObject()));
    }
    pair<long, long> scanRange{0, partition->getNumBytes()};

    // project column names and its union with project column names
    vector<string> predicateColumnNames;
    if (predicate) {
      const auto predicateColumnNameSet = predicate->involvedColumnNames();
      predicateColumnNames.assign(predicateColumnNameSet.begin(), predicateColumnNameSet.end());
    }
    const auto &projPredColumnNames = union_(projectColumnNames, predicateColumnNames);

    // weighted segment keys
    vector<shared_ptr<SegmentKey>> weightedSegmentKeys;
    weightedSegmentKeys.reserve(projPredColumnNames.size());
    for (const auto &weightedColumnName: projPredColumnNames) {
      weightedSegmentKeys.emplace_back(
              SegmentKey::make(partition, weightedColumnName, SegmentRange::make(scanRange.first, scanRange.second)));
    }

    // cache load
    const auto cacheLoadPOp = make_shared<cache::CacheLoadPOp>(fmt::format("CacheLoad[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                               projectColumnNames,
                                                               partitionId % numComputeNodes_,
                                                               predicateColumnNames,
                                                               std::vector<std::set<std::string>>{},    // not needed
                                                               projPredColumnNames,
                                                               partition,
                                                               scanRange.first,
                                                               scanRange.second,
                                                               fpdbStoreConnector_);
    allPOps.emplace_back(cacheLoadPOp);

    // remote file scan
    const auto &scanPOp = make_shared<file::RemoteFileScanPOp>(fmt::format("RemoteFileScan[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                               projPredColumnNames,
                                                               partitionId % numComputeNodes_,
                                                               bucket,
                                                               object,
                                                               table->getFormat(),
                                                               table->getSchema(),
                                                               partition->getNumBytes(),
                                                               fpdbStoreConnector_->getHost(*fpdbStoreNodeId),
                                                               fpdbStoreConnector_->getFileServicePort(),
                                                               nullopt,
                                                               false,
                                                               true);
    allPOps.emplace_back(scanPOp);

    // merge
    const auto &mergePOp = make_shared<merge::MergePOp>(fmt::format("Merge[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                        projPredColumnNames,
                                                        partitionId % numComputeNodes_);
    allPOps.emplace_back(mergePOp);

    // connect
    cacheLoadPOp->setHitOperator(mergePOp);
    cacheLoadPOp->setMissOperatorToCache(scanPOp);
    scanPOp->produce(mergePOp);
    scanPOp->consume(cacheLoadPOp);
    mergePOp->setLeftProducer(cacheLoadPOp);
    mergePOp->setRightProducer(scanPOp);

    // filter
    if (predicate) {
      const auto &filterPOp = make_shared<filter::FilterPOp>(fmt::format("Filter[{}]-{}/{}", separableSuperPrePOp_->getId(), bucket, object),
                                                             projectColumnNames,
                                                             partitionId % numComputeNodes_,
                                                             predicate,
                                                             table,
                                                             weightedSegmentKeys);
      allPOps.emplace_back(filterPOp);
      mergePOp->produce(filterPOp);
      filterPOp->consume(mergePOp);
      selfConnDownPOps.emplace_back(filterPOp);
    } else {
      selfConnDownPOps.emplace_back(mergePOp);
    }

    ++partitionId;
  }

  return make_pair(selfConnDownPOps, allPOps);
}

// majorly a copy from 'PrePToPTransformer::transformProject()'
pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformProject(const shared_ptr<ProjectPrePOp> &projectPrePOp) {
  // id
  auto prePOpId = projectPrePOp->getId();

  // transform producers
  const auto &producersTransRes = transformProducers(projectPrePOp);
  if (producersTransRes.size() != 1) {
    throw runtime_error(fmt::format("Unsupported number of producers for project, should be {}, but get {}",
                                    1, producersTransRes.size()));
  }

  // transform self
  const auto &producerTransRes = producersTransRes[0];
  auto upConnPOps = producerTransRes.first;
  auto allPOps = producerTransRes.second;

  vector<shared_ptr<PhysicalOp>> selfPOps;
  vector<string> projectColumnNames{projectPrePOp->getProjectColumnNames().begin(),
                                    projectPrePOp->getProjectColumnNames().end()};
  for (size_t i = 0; i < upConnPOps.size(); ++i) {
    auto projectPOp = make_shared<project::ProjectPOp>(fmt::format("Project[{}]-{}", prePOpId, i),
                                                       projectColumnNames,
                                                       upConnPOps[i]->getNodeId(),
                                                       projectPrePOp->getExprs(),
                                                       projectPrePOp->getExprNames(),
                                                       projectPrePOp->getProjectColumnNamePairs());
    selfPOps.emplace_back(projectPOp);
    // update for hash join pushdown
    if (hashJoinTransInfo_.enabled_) {
      hashJoinTransInfo_.opToStoreNode_[projectPOp->name()] =
              hashJoinTransInfo_.opToStoreNode_[upConnPOps[i]->name()];
    }
  }

  // connect to upstream
  PrePToPTransformerUtil::connectOneToOne(upConnPOps, selfPOps);

  // collect all physical ops
  allPOps.insert(allPOps.end(), selfPOps.begin(), selfPOps.end());

  return make_pair(selfPOps, allPOps);
}

// majorly a copy from 'PrePToPTransformer::transformFilter()'
pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformFilter(const shared_ptr<FilterPrePOp> &filterPrePOp) {
  // id
  auto prePOpId = filterPrePOp->getId();

  // transform producers
  const auto &producersTransRes = transformProducers(filterPrePOp);
  if (producersTransRes.size() != 1) {
    throw runtime_error(fmt::format("Unsupported number of producers for filter, should be {}, but get {}",
                                    1, producersTransRes.size()));
  }

  // transform self
  const auto &producerTransRes = producersTransRes[0];
  auto upConnPOps = producerTransRes.first;
  auto allPOps = producerTransRes.second;

  vector<shared_ptr<PhysicalOp>> selfPOps;
  vector<string> projectColumnNames{filterPrePOp->getProjectColumnNames().begin(),
                                    filterPrePOp->getProjectColumnNames().end()};
  for (size_t i = 0; i < upConnPOps.size(); ++i) {
    auto filterPOp = make_shared<filter::FilterPOp>(fmt::format("Filter[{}]-{}", prePOpId, i),
                                                    projectColumnNames,
                                                    upConnPOps[i]->getNodeId(),
                                                    filterPrePOp->getPredicate(),
                                                    nullptr);
    selfPOps.emplace_back(filterPOp);
    // update for hash join pushdown
    if (hashJoinTransInfo_.enabled_) {
      hashJoinTransInfo_.opToStoreNode_[filterPOp->name()] =
              hashJoinTransInfo_.opToStoreNode_[upConnPOps[i]->name()];
    }
  }

  // connect to upstream
  PrePToPTransformerUtil::connectOneToOne(upConnPOps, selfPOps);

  // collect all physical ops
  allPOps.insert(allPOps.end(), selfPOps.begin(), selfPOps.end());

  return make_pair(selfPOps, allPOps);
}

// majorly a copy from 'PrePToPTransformer::transformAggregate()'
pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformAggregate(const shared_ptr<AggregatePrePOp> &aggregatePrePOp) {
  // id
  auto prePOpId = aggregatePrePOp->getId();

  // transform producers
  const auto &producersTransRes = transformProducers(aggregatePrePOp);
  if (producersTransRes.size() != 1) {
    throw runtime_error(fmt::format("Unsupported number of producers for aggregate, should be {}, but get {}",
                                    1, producersTransRes.size()));
  }

  // transform self
  const auto &producerTransRes = producersTransRes[0];
  auto upConnPOps = producerTransRes.first;
  auto allPOps = producerTransRes.second;
  bool isParallel = upConnPOps.size() > 1;

  vector<shared_ptr<PhysicalOp>> aggregatePOps;
  vector<string> projectColumnNames{aggregatePrePOp->getProjectColumnNames().begin(),
                                    aggregatePrePOp->getProjectColumnNames().end()};
  vector<string> parallelAggProjectColumnNames;
  if (isParallel) {
    for (uint i = 0; i < aggregatePrePOp->getFunctions().size(); ++i) {
      const auto prePFunction = aggregatePrePOp->getFunctions()[i];
      const auto aggOutputColumnName = aggregatePrePOp->getAggOutputColumnNames()[i];
      if (prePFunction->getType() == AggregatePrePFunctionType::AVG) {
        parallelAggProjectColumnNames.emplace_back(AggregatePrePFunction::AVG_INTERMEDIATE_SUM_COLUMN_PREFIX + aggOutputColumnName);
        parallelAggProjectColumnNames.emplace_back(AggregatePrePFunction::AVG_INTERMEDIATE_COUNT_COLUMN_PREFIX + aggOutputColumnName);
      } else {
        parallelAggProjectColumnNames.emplace_back(aggOutputColumnName);
      }
    }
  } else {
    parallelAggProjectColumnNames = projectColumnNames;
  }

  for (size_t i = 0; i < upConnPOps.size(); ++i) {
    // aggregate functions, better to let each operator has its own copy of aggregate functions
    vector<shared_ptr<aggregate::AggregateFunction>> aggFunctions;
    for (size_t j = 0; j < aggregatePrePOp->getFunctions().size(); ++j) {
      const auto &prePFunction = aggregatePrePOp->getFunctions()[j];
      const auto &aggOutputColumnName = aggregatePrePOp->getAggOutputColumnNames()[j];
      const auto &transAggFunctions = PrePToPTransformerUtil::transformAggFunction(aggOutputColumnName,
                                                                                   prePFunction,
                                                                                   isParallel);
      aggFunctions.insert(aggFunctions.end(), transAggFunctions.begin(), transAggFunctions.end());
    }

    auto aggregatePOp = make_shared<aggregate::AggregatePOp>(
            fmt::format("Aggregate[{}]-{}", prePOpId, i),
            parallelAggProjectColumnNames,
            upConnPOps[i]->getNodeId(),
            aggFunctions);
    aggregatePOps.emplace_back(aggregatePOp);
    // update for hash join pushdown
    if (hashJoinTransInfo_.enabled_) {
      hashJoinTransInfo_.opToStoreNode_[aggregatePOp->name()] =
              hashJoinTransInfo_.opToStoreNode_[upConnPOps[i]->name()];
    }
  }
  allPOps.insert(allPOps.end(), aggregatePOps.begin(), aggregatePOps.end());

  // connect to upstream
  PrePToPTransformerUtil::connectOneToOne(upConnPOps, aggregatePOps);

  return make_pair(aggregatePOps, allPOps);
}

pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformHashJoin(const shared_ptr<HashJoinPrePOp> &hashJoinPrePOp) {
  if (!USE_ARROW_HASH_JOIN_IMPL) {
    throw runtime_error("Old hash join impl is deprecated");
  }
  switch (mode_->id()) {
    case PULL_UP:
    case CACHING_ONLY: {
      return transformHashJoinNoPushdown(hashJoinPrePOp);
    }
    case PUSHDOWN_ONLY: {
      return transformHashJoinPushdown(hashJoinPrePOp);
    }
    default: {
      throw runtime_error("Hybrid mode for hash join pushdown is not supported");
    }
  }
}

// majorly a copy from 'PrePToPTransformer::transformHashJoin()'
pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformHashJoinNoPushdown(const shared_ptr<HashJoinPrePOp> &hashJoinPrePOp) {
  // id
  auto prePOpId = hashJoinPrePOp->getId();

  // transform producers
  const auto &producersTransRes = transformProducers(hashJoinPrePOp);
  if (producersTransRes.size() != 2) {
    throw runtime_error(fmt::format("Unsupported number of producers for hashJoin, should be {}, but get {}",
                                    2, producersTransRes.size()));
  }
  auto upLeftConnPOps = producersTransRes[0].first;
  auto upRightConnPOps = producersTransRes[1].first;
  auto leftAllPOps = producersTransRes[0].second;
  auto rightAllPOps = producersTransRes[1].second;
  auto allPOps = leftAllPOps;
  allPOps.insert(allPOps.end(), rightAllPOps.begin(), rightAllPOps.end());
  
  // information from prep op
  vector<string> projectColumnNames{hashJoinPrePOp->getProjectColumnNames().begin(),
                                    hashJoinPrePOp->getProjectColumnNames().end()};
  auto joinType = hashJoinPrePOp->getJoinType();
  const auto &leftColumnNames = hashJoinPrePOp->getLeftColumnNames();
  const auto &rightColumnNames = hashJoinPrePOp->getRightColumnNames();
  join::HashJoinPredicate hashJoinPredicate(leftColumnNames, rightColumnNames);
  const auto &hashJoinPredicateStr = hashJoinPredicate.toString();
  
  // shuffle
  vector<shared_ptr<PhysicalOp>> shuffleLeftPOps, shuffleRightPOps;
  for (const auto &upLeftConnPOp: upLeftConnPOps) {
    shuffleLeftPOps.emplace_back(make_shared<shuffle::ShufflePOp>(
            fmt::format("Shuffle[{}]-{}", prePOpId, upLeftConnPOp->name()),
            upLeftConnPOp->getProjectColumnNames(),
            upLeftConnPOp->getNodeId(),
            leftColumnNames));
  }
  for (const auto &upRightConnPOp : upRightConnPOps) {
    shuffleRightPOps.emplace_back(make_shared<shuffle::ShufflePOp>(
            fmt::format("Shuffle[{}]-{}", prePOpId, upRightConnPOp->name()),
            upRightConnPOp->getProjectColumnNames(),
            upRightConnPOp->getNodeId(),
            rightColumnNames));
  }
  allPOps.insert(allPOps.end(), shuffleLeftPOps.begin(), shuffleLeftPOps.end());
  allPOps.insert(allPOps.end(), shuffleRightPOps.begin(), shuffleRightPOps.end());
  PrePToPTransformerUtil::connectOneToOne(upLeftConnPOps, shuffleLeftPOps);
  PrePToPTransformerUtil::connectOneToOne(upRightConnPOps, shuffleRightPOps);
  
  // bloom filter if enabled
  vector<shared_ptr<PhysicalOp>> bloomFilterCreatePOps, bloomFilterUsePOps;
  bool useBloomFilter = USE_BLOOM_FILTER &&
          (joinType == JoinType::INNER || joinType == JoinType::LEFT || joinType == JoinType::SEMI);
  if (useBloomFilter) {
    for (int i = 0; i < computeParallelDegree_ * numComputeNodes_; ++i) {
      auto bloomFilterCreatePOp = make_shared<bloomfilter::BloomFilterCreatePOp>(
              fmt::format("BloomFilterCreate[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
              upLeftConnPOps[0]->getProjectColumnNames(),
              i % numComputeNodes_,
              leftColumnNames);
      auto bloomFilterUsePOp = make_shared<bloomfilter::BloomFilterUsePOp>(
              fmt::format("BloomFilterUse[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
              upRightConnPOps[0]->getProjectColumnNames(),
              i % numComputeNodes_,
              rightColumnNames);
      bloomFilterCreatePOps.emplace_back(bloomFilterCreatePOp);
      bloomFilterUsePOps.emplace_back(bloomFilterUsePOp);
      bloomFilterCreatePOp->addBloomFilterUsePOp(bloomFilterUsePOp);
      bloomFilterUsePOp->consume(bloomFilterCreatePOp);
    }
    allPOps.insert(allPOps.end(), bloomFilterCreatePOps.begin(), bloomFilterCreatePOps.end());
    allPOps.insert(allPOps.end(), bloomFilterUsePOps.begin(), bloomFilterUsePOps.end());
    PrePToPTransformerUtil::connectManyToMany(shuffleLeftPOps, bloomFilterCreatePOps);
    PrePToPTransformerUtil::connectManyToMany(shuffleRightPOps, bloomFilterUsePOps);
  }

  // hash join
  vector<shared_ptr<PhysicalOp>> hashJoinArrowPOps;
  for (int i = 0; i < computeParallelDegree_ * numComputeNodes_; ++i) {
    auto hashJoinArrowPOp = make_shared<join::HashJoinArrowPOp>(
            fmt::format("HashJoinArrow[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
            projectColumnNames,
            i % numComputeNodes_,
            hashJoinPredicate,
            joinType);
    hashJoinArrowPOps.emplace_back(hashJoinArrowPOp);
    if (USE_BLOOM_FILTER) {
      bloomFilterCreatePOps[i]->produce(hashJoinArrowPOp);
      bloomFilterUsePOps[i]->produce(hashJoinArrowPOp);
      hashJoinArrowPOp->addBuildProducer(bloomFilterCreatePOps[i]);
      hashJoinArrowPOp->addProbeProducer(bloomFilterUsePOps[i]);
    } else {
      for (const auto &shuffleLeftPOp: shuffleLeftPOps) {
        shuffleLeftPOp->produce(hashJoinArrowPOp);
        hashJoinArrowPOp->addBuildProducer(shuffleLeftPOp);
      }
      for (const auto &shuffleRightPOp: shuffleRightPOps) {
        shuffleRightPOp->produce(hashJoinArrowPOp);
        hashJoinArrowPOp->addProbeProducer(shuffleRightPOp);
      }
    }
  }
  allPOps.insert(allPOps.end(), hashJoinArrowPOps.begin(), hashJoinArrowPOps.end());

  return {hashJoinArrowPOps, allPOps};
}

// the major difference is that joins are separately processed in each storage node
pair<vector<shared_ptr<PhysicalOp>>, vector<shared_ptr<PhysicalOp>>>
PrePToFPDBStorePTransformer::transformHashJoinPushdown(const shared_ptr<HashJoinPrePOp> &hashJoinPrePOp) {
  // set flag
  hashJoinTransInfo_.enabled_ = true;

  // id
  auto prePOpId = hashJoinPrePOp->getId();

  // transform producers
  const auto &producersTransRes = transformProducers(hashJoinPrePOp);
  if (producersTransRes.size() != 2) {
    throw runtime_error(fmt::format("Unsupported number of producers for hashJoin, should be {}, but get {}",
                                    2, producersTransRes.size()));
  }
  auto upLeftConnPOps = producersTransRes[0].first;
  auto upRightConnPOps = producersTransRes[1].first;
  auto leftAllPOps = producersTransRes[0].second;
  auto rightAllPOps = producersTransRes[1].second;
  auto allPOps = leftAllPOps;
  allPOps.insert(allPOps.end(), rightAllPOps.begin(), rightAllPOps.end());

  // information from prep op
  vector<string> projectColumnNames{hashJoinPrePOp->getProjectColumnNames().begin(),
                                    hashJoinPrePOp->getProjectColumnNames().end()};
  auto joinType = hashJoinPrePOp->getJoinType();
  const auto &leftColumnNames = hashJoinPrePOp->getLeftColumnNames();
  const auto &rightColumnNames = hashJoinPrePOp->getRightColumnNames();
  join::HashJoinPredicate hashJoinPredicate(leftColumnNames, rightColumnNames);
  const auto &hashJoinPredicateStr = hashJoinPredicate.toString();

  // group upConnPOps by storage nodes
  unordered_map<string, shared_ptr<PhysicalOp>> leftOpMap, rightOpMap;
  for (const auto &leftOp: leftAllPOps) {
    leftOpMap[leftOp->name()] = leftOp;
  }
  for (const auto &rightOp: rightAllPOps) {
    rightOpMap[rightOp->name()] = rightOp;
  }
  unordered_map<string, int> hostToId;
  const auto &hosts = fpdbStoreConnector_->getHosts();
  for (int i = 0; i < (int) hosts.size(); ++i) {
    hostToId[hosts[i]] = i;
  }
  for (const auto &upLeftConnPOp: upLeftConnPOps) {
    PrePToPTransformerUtil::updateOpToStoreNode(hashJoinTransInfo_.opToStoreNode_, upLeftConnPOp, leftOpMap,
                                                objectToHost_, hostToId);
  }
  for (const auto &upRightConnPOp: upRightConnPOps) {
    PrePToPTransformerUtil::updateOpToStoreNode(hashJoinTransInfo_.opToStoreNode_, upRightConnPOp, rightOpMap,
                                                objectToHost_, hostToId);
  }

  // shuffle
  vector<vector<shared_ptr<PhysicalOp>>> shuffleLeftPOps{numFPDBStoreNodes_}, shuffleRightPOps{numFPDBStoreNodes_};
  for (auto &upLeftConnPOp: upLeftConnPOps) {
    shared_ptr<PhysicalOp> shuffleLeftPOp = make_shared<shuffle::ShufflePOp>(
            fmt::format("Shuffle[{}]-{}", prePOpId, upLeftConnPOp->name()),
            upLeftConnPOp->getProjectColumnNames(),
            0,  // never used
            leftColumnNames);
    PrePToPTransformerUtil::connectOneToOne(upLeftConnPOp, shuffleLeftPOp);
    int fpdbStoreNodeId = hashJoinTransInfo_.opToStoreNode_[upLeftConnPOp->name()];
    hashJoinTransInfo_.opToStoreNode_[shuffleLeftPOp->name()] = fpdbStoreNodeId;
    shuffleLeftPOps[fpdbStoreNodeId].emplace_back(shuffleLeftPOp);
  }
  for (auto &upRightConnPOp : upRightConnPOps) {
    shared_ptr<PhysicalOp> shuffleRightPOp = make_shared<shuffle::ShufflePOp>(
            fmt::format("Shuffle[{}]-{}", prePOpId, upRightConnPOp->name()),
            upRightConnPOp->getProjectColumnNames(),
            0,  // never used
            rightColumnNames);
    PrePToPTransformerUtil::connectOneToOne(upRightConnPOp, shuffleRightPOp);
    int fpdbStoreNodeId = hashJoinTransInfo_.opToStoreNode_[upRightConnPOp->name()];
    hashJoinTransInfo_.opToStoreNode_[shuffleRightPOp->name()] = fpdbStoreNodeId;
    shuffleRightPOps[fpdbStoreNodeId].emplace_back(shuffleRightPOp);
  }
  for (int i = 0; i < (int) numFPDBStoreNodes_; ++i) {
    allPOps.insert(allPOps.end(), shuffleLeftPOps[i].begin(), shuffleLeftPOps[i].end());
    allPOps.insert(allPOps.end(), shuffleRightPOps[i].begin(), shuffleRightPOps[i].end());
  }

  // bloom filter if enabled
  vector<vector<shared_ptr<PhysicalOp>>> bloomFilterCreatePOps{numFPDBStoreNodes_},
          bloomFilterUsePOps{numFPDBStoreNodes_};
  bool useBloomFilter = USE_BLOOM_FILTER &&
          (joinType == JoinType::INNER || joinType == JoinType::LEFT || joinType == JoinType::SEMI);
  if (useBloomFilter) {
    for (int i = 0; i < fpdbStoreParallelDegree_ * (int) numFPDBStoreNodes_; ++i) {
      int fpdbStoreNodeId = i / fpdbStoreParallelDegree_;
      auto bloomFilterCreatePOp = make_shared<bloomfilter::BloomFilterCreatePOp>(
              fmt::format("BloomFilterCreate[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
              upLeftConnPOps[0]->getProjectColumnNames(),
              0,  // never used
              leftColumnNames);
      auto bloomFilterUsePOp = make_shared<bloomfilter::BloomFilterUsePOp>(
              fmt::format("BloomFilterUse[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
              upRightConnPOps[0]->getProjectColumnNames(),
              0,  // never used
              rightColumnNames);
      bloomFilterCreatePOps[fpdbStoreNodeId].emplace_back(bloomFilterCreatePOp);
      bloomFilterUsePOps[fpdbStoreNodeId].emplace_back(bloomFilterUsePOp);
      bloomFilterCreatePOp->addBloomFilterUsePOp(bloomFilterUsePOp);
      bloomFilterUsePOp->consume(bloomFilterCreatePOp);
      hashJoinTransInfo_.opToStoreNode_[bloomFilterCreatePOp->name()] = fpdbStoreNodeId;
      hashJoinTransInfo_.opToStoreNode_[bloomFilterUsePOp->name()] = fpdbStoreNodeId;
    }
    for (int i = 0; i < (int) numFPDBStoreNodes_; ++i) {
      allPOps.insert(allPOps.end(), bloomFilterCreatePOps[i].begin(), bloomFilterCreatePOps[i].end());
      allPOps.insert(allPOps.end(), bloomFilterUsePOps[i].begin(), bloomFilterUsePOps[i].end());
      PrePToPTransformerUtil::connectManyToMany(shuffleLeftPOps[i], bloomFilterCreatePOps[i]);
      PrePToPTransformerUtil::connectManyToMany(shuffleRightPOps[i], bloomFilterUsePOps[i]);
    }
  }

  // hash join
  vector<vector<shared_ptr<PhysicalOp>>> hashJoinArrowPOps{numFPDBStoreNodes_};
  vector<shared_ptr<PhysicalOp>> hashJoinArrowPOpsPlain;
  for (int i = 0; i < fpdbStoreParallelDegree_ * (int) numFPDBStoreNodes_; ++i) {
    int fpdbStoreNodeId = i / fpdbStoreParallelDegree_;
    auto hashJoinArrowPOp = make_shared<join::HashJoinArrowPOp>(
            fmt::format("HashJoinArrow[{}]-{}-{}", prePOpId, hashJoinPredicateStr, i),
            projectColumnNames,
            0,  // never used
            hashJoinPredicate,
            joinType);
    hashJoinArrowPOps[fpdbStoreNodeId].emplace_back(hashJoinArrowPOp);
    hashJoinTransInfo_.opToStoreNode_[hashJoinArrowPOp->name()] = fpdbStoreNodeId;
  }
  for (int i = 0; i < (int) numFPDBStoreNodes_; ++i) {
    hashJoinArrowPOpsPlain.insert(hashJoinArrowPOpsPlain.end(), hashJoinArrowPOps[i].begin(),
                                  hashJoinArrowPOps[i].end());
    allPOps.insert(allPOps.end(), hashJoinArrowPOps[i].begin(), hashJoinArrowPOps[i].end());
    if (USE_BLOOM_FILTER) {
      for (int j = 0; j < fpdbStoreParallelDegree_; ++j) {
        bloomFilterCreatePOps[i][j]->produce(hashJoinArrowPOps[i][j]);
        bloomFilterUsePOps[i][j]->produce(hashJoinArrowPOps[i][j]);
        static_pointer_cast<join::HashJoinArrowPOp>(hashJoinArrowPOps[i][j])
                ->addBuildProducer(bloomFilterCreatePOps[i][j]);
        static_pointer_cast<join::HashJoinArrowPOp>(hashJoinArrowPOps[i][j])
                ->addProbeProducer(bloomFilterUsePOps[i][j]);
      }
    } else {
      for (const auto &shuffleLeftPOp: shuffleLeftPOps[i]) {
        for (const auto &hashJoinArrowPOp: hashJoinArrowPOps[i]) {
          shuffleLeftPOp->produce(hashJoinArrowPOp);
          static_pointer_cast<join::HashJoinArrowPOp>(hashJoinArrowPOp)->addBuildProducer(shuffleLeftPOp);
        }
      }
      for (const auto &shuffleRightPOp: shuffleRightPOps[i]) {
        for (const auto &hashJoinArrowPOp: hashJoinArrowPOps[i]) {
          shuffleRightPOp->produce(hashJoinArrowPOp);
          static_pointer_cast<join::HashJoinArrowPOp>(hashJoinArrowPOp)->addProbeProducer(shuffleRightPOp);
        }
      }
    }
  }

  return {hashJoinArrowPOpsPlain, allPOps};
}

}

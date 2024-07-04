//
// Created by Yifei Yang on 11/23/21.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_EXECUTOR_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_EXECUTOR_H

#include <fpdb/executor/physical/PhysicalPlan.h>
#include <fpdb/cache/policy/CachingPolicy.h>
#include <fpdb/plan/Mode.h>
#include <fpdb/catalogue/obj-store/fpdb-store/FPDBStoreConnector.h>
#include <fpdb/tuple/TupleSet.h>
#include <caf/all.hpp>
#include <memory>

using namespace fpdb::executor::physical;
using namespace fpdb::cache::policy;
using namespace fpdb::plan;
using namespace fpdb::tuple;
using namespace std;

namespace fpdb::executor {

/**
 * Query executor
 */
class Executor {

public:
  Executor(const shared_ptr<::caf::actor_system> &actorSystem,
           const vector<::caf::node_id> &nodes,
           const shared_ptr<Mode> &mode,
           const shared_ptr<CachingPolicy> &cachingPolicy,
           bool showOpTimes,
           bool showScanMetrics);
  ~Executor();

  /**
   * Start and stop
   */
  void start();
  void stop();

  /**
   * Execute a physical plan
   * @param physicalPlan
   * @param isDistributed
   *
   * @return query result and execution time
   */
  pair<shared_ptr<TupleSet>, long> execute(
          long queryId,
          const shared_ptr<PhysicalPlan> &physicalPlan,
          bool isDistributed,
          bool collAdaptPushdownMetrics = false,
          const std::shared_ptr<fpdb::catalogue::obj_store::FPDBStoreConnector> &fpdbStoreConnector = nullptr);

  const ::caf::actor &getLocalSegmentCacheActor() const;
  const vector<::caf::actor> &getRemoteSegmentCacheActors() const;
  const ::caf::actor &getRemoteSegmentCacheActor(int nodeId) const;
  const shared_ptr<::caf::actor_system> &getActorSystem() const;

  /**
   * Metrics
   */
  std::string showCacheMetrics();
  void clearCacheMetrics();
  double getCrtQueryHitRatio();
  double getCrtQueryShardHitRatio();

private:
  bool isCacheUsed();

  shared_ptr<::caf::actor_system> actorSystem_;
  vector<::caf::node_id> nodes_;
  unique_ptr<::caf::scoped_actor> rootActor_;
  ::caf::actor localSegmentCacheActor_;             // used in single-node execution
  vector<::caf::actor> remoteSegmentCacheActors_;   // used in distributed execution
  shared_ptr<CachingPolicy> cachingPolicy_;
  shared_ptr<Mode> mode_;
  bool running_;

  bool showOpTimes_;
  bool showScanMetrics_;
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_EXECUTOR_H

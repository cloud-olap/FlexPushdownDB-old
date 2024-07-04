//
// Created by Yifei Yang on 11/30/21.
//

#ifndef FPDB_FPDB_MAIN_TEST_BASE_TESTUTIL_H
#define FPDB_FPDB_MAIN_TEST_BASE_TESTUTIL_H

#include <fpdb/main/ActorSystemConfig.h>
#include <fpdb/executor/Executor.h>
#include <fpdb/store/server/Server.hpp>
#include <fpdb/store/client/FPDBStoreClientConfig.h>
#include <fpdb/catalogue/Catalogue.h>
#include <fpdb/catalogue/CatalogueEntry.h>
#include <fpdb/catalogue/obj-store/ObjStoreConnector.h>
#include <fpdb/catalogue/obj-store/ObjStoreType.h>
#include <fpdb/calcite/CalciteClient.h>
#include <fpdb/aws/AWSClient.h>
#include <memory>
#include <vector>
#include <string>

using namespace fpdb::main;
using namespace fpdb::executor;
using namespace fpdb::plan;
using namespace fpdb::cache;
using namespace fpdb::catalogue;
using namespace fpdb::catalogue::obj_store;
using namespace fpdb::calcite;
using namespace fpdb::aws;
using namespace std;

namespace fpdb::main::test {

class TestUtil {

public:
  /**
   * Test with calcite server already started, using pullup by default
   * @param schemaName
   * @param queryFileNames
   * @param parallelDegree
   * @param isDistributed
   *
   * @return whether executed successfully
   */
  static bool e2eNoStartCalciteServer(const string &schemaName,
                                      const vector<string> &queryFileNames,
                                      int parallelDegree,
                                      bool isDistributed,
                                      ObjStoreType objStoreType,
                                      const shared_ptr<Mode> &mode = Mode::pullupMode(),
                                      CachingPolicyType cachingPolicyType = CachingPolicyType::NONE,
                                      size_t cacheSize = 1L * 1024 * 1024 * 1024);

  /**
   * Temp test for disabling heuristic join ordering by calcite
   */
  static bool e2eNoStartCalciteServerNoHeuristicJoinOrdering(
          const string &schemaName,
          const vector<string> &queryFileNames,
          int parallelDegree,
          bool isDistributed,
          ObjStoreType objStoreType,
          const shared_ptr<Mode> &mode = Mode::pullupMode(),
          CachingPolicyType cachingPolicyType = CachingPolicyType::NONE,
          size_t cacheSize = 1L * 1024 * 1024 * 1024);

  static void writeQueryToFile(const std::string queryFileName, const std::string query);
  static void removeQueryFile(const std::string queryFileName);

  static void startFPDBStoreServer();
  static void stopFPDBStoreServer();

  TestUtil(const string &schemaName,
           const vector<string> &queryFileNames,
           int parallelDegree,
           bool isDistributed,
           ObjStoreType objStoreType,
           const shared_ptr<Mode> &mode = Mode::pullupMode(),
           CachingPolicyType cachingPolicyType = CachingPolicyType::NONE,
           size_t cacheSize = 1L * 1024 * 1024 * 1024);

  double getCrtQueryHitRatio() const;
  void setUseHeuristicJoinOrdering(bool useHeuristicJoinOrdering);
  void setFixLayoutIndices(const set<int> &fixLayoutIndices);
  void setCollAdaptPushdownMetrics(bool collAdaptPushdownMetrics);
  void setConcurrent(bool concurrent);
  void setShowResults(bool showResults);

  void runTest();

private:
  void readPushdownFlags();
  void makeObjStoreConnector();
  void makeCatalogueEntry();
  void makeCachingPolicy();
  void makeCalciteClient();
  void connect();
  void makeExecutor();
  void executeQueryFile(long queryId, const string &queryFileName);
  void stop();

  static std::shared_ptr<fpdb::store::server::Server> fpdbStoreServer_;
  static std::shared_ptr<fpdb::store::server::caf::ActorManager> actorManager_;
  static std::shared_ptr<fpdb::store::client::FPDBStoreClientConfig> fpdbStoreClientConfig_;

  // input parameters
  std::string schemaName_;
  vector<string> queryFileNames_;
  int parallelDegree_;
  bool isDistributed_;
  ObjStoreType objStoreType_;
  shared_ptr<Mode> mode_;
  CachingPolicyType cachingPolicyType_;
  size_t cacheSize_;

  // internal parameters
  shared_ptr<CachingPolicy> cachingPolicy_;
  shared_ptr<ObjStoreConnector> objStoreConnector_;
  shared_ptr<Catalogue> catalogue_;
  shared_ptr<CatalogueEntry> catalogueEntry_;
  shared_ptr<CalciteClient> calciteClient_;
  shared_ptr<ActorSystemConfig> actorSystemConfig_;
  shared_ptr<::caf::actor_system> actorSystem_;
  vector<::caf::node_id> nodes_;
  shared_ptr<Executor> executor_;
  shared_ptr<std::atomic<long>> queryCounter_;

  // whether to use heuristic join ordering by calcite, true by default
  bool useHeuristicJoinOrdering_ = true;

  // used to fix cache layout for testing
  set<int> fixLayoutIndices_;

  // metrics used for checking in some unit tests
  double crtQueryHitRatio_;

  // used to collect adaptive pushdown metrics
  bool collAdaptPushdownMetrics_ = false;

  // whether to run queries concurrently
  bool concurrent_ = false;

  // whether to show query results
  bool showResults_ = false;
};

}

#endif //FPDB_FPDB_MAIN_TEST_BASE_TESTUTIL_H

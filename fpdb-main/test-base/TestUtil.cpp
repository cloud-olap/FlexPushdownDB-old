//
// Created by Yifei Yang on 11/30/21.
//

#include "TestUtil.h"
#include <fpdb/main/ExecConfig.h>
#include <fpdb/executor/caf/CAFInit.h>
#include <fpdb/executor/caf/CAFAdaptPushdownUtil.h>
#include <fpdb/executor/physical/transform/PrePToPTransformer.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/Globals.h>
#include <fpdb/cache/Globals.h>
#include <fpdb/cache/policy/LRUCachingPolicy.h>
#include <fpdb/cache/policy/LFUCachingPolicy.h>
#include <fpdb/cache/policy/LFUSCachingPolicy.h>
#include <fpdb/cache/policy/WLFUCachingPolicy.h>
#include <fpdb/calcite/CalciteConfig.h>
#include <fpdb/plan/calcite/CalcitePlanJsonDeserializer.h>
#include <fpdb/plan/prephysical/separable/SeparablePrePOpTransformer.h>
#include <fpdb/plan/prephysical/separable/Globals.h>
#include <fpdb/catalogue/obj-store/ObjStoreCatalogueEntryReader.h>
#include <fpdb/catalogue/obj-store/s3/S3Connector.h>
#include <fpdb/catalogue/obj-store/fpdb-store/FPDBStoreConnector.h>
#include <fpdb/aws/AWSConfig.h>
#include <fpdb/store/client/FPDBStoreClientConfig.h>
#include <fpdb/store/server/flight/Util.hpp>
#include <fpdb/util/Util.h>

using namespace fpdb::executor::physical;
using namespace fpdb::cache;
using namespace fpdb::plan::calcite;
using namespace fpdb::plan::prephysical::separable;
using namespace fpdb::catalogue::obj_store;
using namespace fpdb::store::client;
using namespace fpdb::store::server::flight;
using namespace fpdb::util;

namespace fpdb::main::test {

TestUtil::TestUtil(const string &schemaName,
                   const vector<string> &queryFileNames,
                   int parallelDegree,
                   bool isDistributed,
                   ObjStoreType objStoreType,
                   const shared_ptr<Mode> &mode,
                   CachingPolicyType cachingPolicyType,
                   size_t cacheSize) :
  schemaName_(schemaName),
  queryFileNames_(queryFileNames),
  parallelDegree_(parallelDegree),
  isDistributed_(isDistributed),
  objStoreType_(objStoreType),
  mode_(mode),
  cachingPolicyType_(cachingPolicyType),
  cacheSize_(cacheSize),
  queryCounter_(make_shared<std::atomic<long>>(0)) {

  // Set pushdown feature flags
  readPushdownFlags();
}

bool TestUtil::e2eNoStartCalciteServer(const string &schemaName,
                                       const vector<string> &queryFileNames,
                                       int parallelDegree,
                                       bool isDistributed,
                                       ObjStoreType objStoreType,
                                       const shared_ptr<Mode> &mode,
                                       CachingPolicyType cachingPolicyType,
                                       size_t cacheSize) {
  TestUtil testUtil(schemaName,
                    queryFileNames,
                    parallelDegree,
                    isDistributed,
                    objStoreType,
                    mode,
                    cachingPolicyType,
                    cacheSize);
  try {
    testUtil.runTest();
    return true;
  } catch (const runtime_error &err) {
    cout << err.what() << endl;
    return false;
  }
}

bool TestUtil::e2eNoStartCalciteServerNoHeuristicJoinOrdering(const string &schemaName,
                                                              const vector<string> &queryFileNames, int parallelDegree,
                                                              bool isDistributed, ObjStoreType objStoreType,
                                                              const shared_ptr<Mode> &mode,
                                                              CachingPolicyType cachingPolicyType, size_t cacheSize) {
  TestUtil testUtil(schemaName,
                    queryFileNames,
                    parallelDegree,
                    isDistributed,
                    objStoreType,
                    mode,
                    cachingPolicyType,
                    cacheSize);
  testUtil.setUseHeuristicJoinOrdering(false);
  try {
    testUtil.runTest();
    return true;
  } catch (const runtime_error &err) {
    cout << err.what() << endl;
    return false;
  }
}

void TestUtil::writeQueryToFile(const std::string queryFileName, const std::string query) {
  std::string queryFilePath = std::filesystem::current_path()
          .parent_path()
          .append("resources/query")
          .append(queryFileName)
          .string();
  writeFile(queryFilePath, query);
}

void TestUtil::removeQueryFile(const std::string queryFileName) {
  std::string queryFilePath = std::filesystem::current_path()
          .parent_path()
          .append("resources/query")
          .append(queryFileName)
          .string();
  std::remove(queryFilePath.c_str());
}

std::shared_ptr<fpdb::store::server::Server> TestUtil::fpdbStoreServer_ = nullptr;
std::shared_ptr<fpdb::store::server::caf::ActorManager> TestUtil::actorManager_= nullptr;
std::shared_ptr<fpdb::store::client::FPDBStoreClientConfig> TestUtil::fpdbStoreClientConfig_= nullptr;

void TestUtil::startFPDBStoreServer() {
  fpdbStoreClientConfig_ = fpdb::store::client::FPDBStoreClientConfig::parseFPDBStoreClientConfig();
  actorManager_ = fpdb::store::server::caf::ActorManager::make<::caf::id_block::Server>().value();
  fpdbStoreServer_ = fpdb::store::server::Server::make(
          fpdb::store::server::ServerConfig{"1",
                                            0,
                                            true,
                                            std::nullopt,
                                            0,
                                            fpdbStoreClientConfig_->getFlightPort(),
                                            fpdbStoreClientConfig_->getFileServicePort(),
                                            "test-resources/fpdb-store",
                                            1},
          std::nullopt,
          actorManager_);
  auto initResult = fpdbStoreServer_->init();
//  REQUIRE(initResult.has_value());
  auto startResult = fpdbStoreServer_->start();
//  REQUIRE(startResult.has_value());
}

void TestUtil::stopFPDBStoreServer() {
  fpdbStoreServer_->stop();
  fpdbStoreServer_.reset();
  actorManager_.reset();
}

double TestUtil::getCrtQueryHitRatio() const {
  return crtQueryHitRatio_;
}

void TestUtil::setUseHeuristicJoinOrdering(bool useHeuristicJoinOrdering) {
  useHeuristicJoinOrdering_ = useHeuristicJoinOrdering;
}

void TestUtil::setFixLayoutIndices(const set<int> &fixLayoutIndices) {
  fixLayoutIndices_ = fixLayoutIndices;
}

void TestUtil::setCollAdaptPushdownMetrics(bool collAdaptPushdownMetrics) {
  collAdaptPushdownMetrics_ = collAdaptPushdownMetrics;
}

void TestUtil::setConcurrent(bool concurrent) {
  concurrent_ = concurrent;
}

void TestUtil::setShowResults(bool showResults) {
  showResults_ = showResults;
}

void TestUtil::runTest() {
  spdlog::set_level(spdlog::level::info);

  // Object store connector
  makeObjStoreConnector();

  // Catalogue entry
  makeCatalogueEntry();

  // Caching policy
  makeCachingPolicy();

  // Calcite client
  makeCalciteClient();

  // create the executor
  makeExecutor();

  // run queries
  std::vector<std::thread> ths;
  for (uint i = 0; i < queryFileNames_.size(); ++i) {
    long queryId = queryCounter_->fetch_add(1);
    if (!concurrent_) {
      executeQueryFile(queryId, queryFileNames_[i]);
      // fix cache layout if needed, only available in serial runs
      if (fixLayoutIndices_.find((int) i) != fixLayoutIndices_.end()) {
        fpdb::cache::FIX_CACHE_LAYOUT = true;
      }
    } else {
      ths.push_back(std::thread(&TestUtil::executeQueryFile, this, queryId, queryFileNames_[i]));
    }
  }

  // wait for concurrent runs
  if (concurrent_) {
    for (auto &th: ths) {
      th.join();
    }
  }

  // stop
  stop();
}

void TestUtil::readPushdownFlags() {
  unordered_map<string, string> configMap = readConfig("pushdown.conf");
  ENABLE_GROUP_BY_PUSHDOWN = parseBool(configMap["GROUP"]);
  ENABLE_SHUFFLE_PUSHDOWN = parseBool(configMap["SHUFFLE"]);
  ENABLE_BLOOM_FILTER_PUSHDOWN = parseBool(configMap["BLOOM_FILTER"]);
  ENABLE_FILTER_BITMAP_PUSHDOWN = parseBool(configMap["FILTER_BITMAP"]);
  ENABLE_CO_LOCATED_JOIN_PUSHDOWN = parseBool(configMap["CO_LOCATED_JOIN"]);
}

void TestUtil::makeObjStoreConnector() {
  switch (objStoreType_) {
    case ObjStoreType::S3: {
      auto awsClient = make_shared<AWSClient>(make_shared<AWSConfig>(fpdb::aws::S3, 0));
      awsClient->init();
      objStoreConnector_ = make_shared<S3Connector>(awsClient);
      return;
    }
    case ObjStoreType::FPDB_STORE: {
      auto fpdbStoreClientConfig = FPDBStoreClientConfig::parseFPDBStoreClientConfig();
      objStoreConnector_ = make_shared<FPDBStoreConnector>(fpdbStoreClientConfig->getHosts(),
                                                           fpdbStoreClientConfig->getFileServicePort(),
                                                           fpdbStoreClientConfig->getFlightPort());
      return;
    }
    default:
      throw runtime_error("Unknown object store type");
  }
}

void TestUtil::makeCatalogueEntry() {
  // create the catalogue
  string s3Bucket = "flexpushdowndb";
  filesystem::path metadataPath = std::filesystem::current_path()
          .parent_path()
          .append("resources/metadata");
  catalogue_ = make_shared<Catalogue>("main", metadataPath);

  // read catalogue entry
  catalogueEntry_ = ObjStoreCatalogueEntryReader::readCatalogueEntry(catalogue_,
                                                                     s3Bucket,
                                                                     schemaName_,
                                                                     objStoreConnector_);
  catalogue_->putEntry(catalogueEntry_);
}

void TestUtil::makeCachingPolicy() {
  switch (cachingPolicyType_) {
    case CachingPolicyType::LFU: {
      cachingPolicy_ = make_shared<LFUCachingPolicy>(cacheSize_, catalogueEntry_);
      return;
    }
    case CachingPolicyType::LFUS: {
      cachingPolicy_ = make_shared<LFUSCachingPolicy>(cacheSize_, catalogueEntry_);
      return;
    }
    case CachingPolicyType::LRU: {
      cachingPolicy_ = make_shared<LRUCachingPolicy>(cacheSize_, catalogueEntry_);
      return;
    }
    case CachingPolicyType::WLFU: {
      cachingPolicy_ = make_shared<WLFUCachingPolicy>(cacheSize_, catalogueEntry_);
      return;
    }
    case CachingPolicyType::NONE: {
      cachingPolicy_ = nullptr;
      return;
    }
    default:
      throw runtime_error(fmt::format("Unknown caching policy: {}", cachingPolicyType_));
  }
}

void TestUtil::makeCalciteClient() {
  auto calciteConfig = CalciteConfig::parseCalciteConfig();
  calciteClient_ = make_shared<CalciteClient>(calciteConfig);
  calciteClient_->startClient();
}

void TestUtil::connect() {
  if (!actorSystemConfig_->nodeIps_.empty()) {
    for (const auto &nodeIp: actorSystemConfig_->nodeIps_) {
      auto expectedNode = actorSystem_->middleman().connect(nodeIp, actorSystemConfig_->port_);
      if (!expectedNode) {
        nodes_.clear();
        throw runtime_error(
                fmt::format("Failed to connected to server {}: {}", nodeIp, to_string(expectedNode.error())));
      }
      nodes_.emplace_back(*expectedNode);
    }
  }
}

void TestUtil::makeExecutor() {
  // create the actor system
  const auto &remoteIps = readRemoteIps();
  int CAFServerPort = ExecConfig::parseCAFServerPort();
  actorSystemConfig_ = make_shared<ActorSystemConfig>(CAFServerPort, remoteIps, false);
  fpdb::executor::caf::CAFInit::initCAFGlobalMetaObjects();
  actorSystem_ = make_shared<::caf::actor_system>(*actorSystemConfig_);

  // create the executor
  if (isDistributed_) {
    connect();
  }
  executor_ = make_shared<Executor>(actorSystem_,
                                    nodes_,
                                    mode_,
                                    cachingPolicy_,
                                    false,
                                    false);
  executor_->start();

  // make actor system for adaptive pushdown if needed
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    fpdb::executor::caf::CAFAdaptPushdownUtil::startDaemonAdaptPushdownActorSystem();
  }
}

void TestUtil::executeQueryFile(long queryId, const string &queryFileName) {
  ConcurrentOutputMutex.lock();
  cout << fmt::format("Query: '{}' (id: {})", queryFileName, queryId) << endl;
  ConcurrentOutputMutex.unlock();

  // Plan query
  string queryPath = std::filesystem::current_path()
          .parent_path()
          .append("resources/query")
          .append(queryFileName)
          .string();
  string query = readFile(queryPath);
  string planResult = calciteClient_->planQuery(query, schemaName_, useHeuristicJoinOrdering_);

  // deserialize plan json string into prephysical plan
  auto prePhysicalPlan = CalcitePlanJsonDeserializer::deserialize(planResult, catalogueEntry_);

  // trim unused fields (Calcite trimmer does not trim completely)
  prePhysicalPlan->populateAndTrimProjectColumns();

  // detect separable operators
  auto separableTransformer = make_shared<SeparablePrePOpTransformer>(catalogueEntry_);
  separableTransformer->transform(prePhysicalPlan);

  // transform prephysical plan to physical plan
  int numNodes = isDistributed_ ? (int) nodes_.size() : 1;
  auto physicalPlan = PrePToPTransformer::transform(prePhysicalPlan,
                                                    catalogueEntry_,
                                                    objStoreConnector_,
                                                    mode_,
                                                    parallelDegree_,
                                                    numNodes);

  // execute
  const auto &execRes = objStoreConnector_->getStoreType() == ObjStoreType::FPDB_STORE ?
                        executor_->execute(queryId, physicalPlan, isDistributed_, collAdaptPushdownMetrics_,
                                           static_pointer_cast<FPDBStoreConnector>(objStoreConnector_)) :
                        executor_->execute(queryId, physicalPlan, isDistributed_);

  // show output
  stringstream ss;
  if (showResults_) {
    ss << fmt::format("\nResult of '{}' (id: {}) |\n{}", queryFileName, queryId, execRes.first->showString(
            TupleSetShowOptions(TupleSetShowOrientation::RowOriented, 100)));
  }
  ss << fmt::format("\nTime (id: {}): {} secs\n\n", queryId, (double) (execRes.second) / 1000000000.0);
  ss << endl;
  ConcurrentOutputMutex.lock();
  cout << ss.str() << endl;
  ConcurrentOutputMutex.unlock();

  // metrics used for checking in some unit tests
  crtQueryHitRatio_ = executor_->getCrtQueryHitRatio();
}

void TestUtil::stop() {
  // need to shutdown awsClient first
  if (objStoreType_ == ObjStoreType::S3) {
    auto s3Connector = static_pointer_cast<S3Connector>(objStoreConnector_);
    s3Connector->getAwsClient()->shutdown();
  }
  executor_->stop();

  // stop calcite client
  calciteClient_->stopClient();

  // stop actor system for adaptive pushdown if needed
  if (ENABLE_ADAPTIVE_PUSHDOWN) {
    fpdb::executor::caf::CAFAdaptPushdownUtil::stopDaemonAdaptPushdownActorSystem();
  }

  // clear global states
  fpdb::executor::clearGlobal();

  // reset flag for cache layout
  fpdb::cache::FIX_CACHE_LAYOUT = false;
}

}

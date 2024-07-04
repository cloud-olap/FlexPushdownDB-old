//
// Created by Yifei Yang on 2/9/21.
//

#include <fpdb/main/Client.h>
#include <fpdb/executor/caf/CAFInit.h>
#include <fpdb/executor/physical/transform/PrePToPTransformer.h>
#include <fpdb/plan/calcite/CalcitePlanJsonDeserializer.h>
#include <fpdb/catalogue/obj-store/ObjStoreCatalogueEntryReader.h>
#include <fpdb/catalogue/obj-store/s3/S3Connector.h>
#include <fpdb/util/Util.h>

using namespace fpdb::plan::calcite;
using namespace fpdb::catalogue::obj_store;
using namespace fpdb::util;

namespace fpdb::main {

path Client::getDefaultMetadataPath() {
  return current_path().parent_path().append("resources/metadata");
}

string Client::getDefaultCatalogueName() {
  return "main";
}

string Client::start() {
  // catalogue
  catalogue_ = make_shared<Catalogue>(getDefaultCatalogueName(), getDefaultMetadataPath());

  // AWS client
  const auto &awsConfig = AWSConfig::parseAWSConfig();
  awsClient_ = make_shared<AWSClient>(awsConfig);
  awsClient_->init();
  SPDLOG_INFO("AWS client started");

  // calcite client
  const auto &calciteConfig = CalciteConfig::parseCalciteConfig();
  calciteClient_ = make_shared<CalciteClient>(calciteConfig);
  calciteClient_->startServer();
  SPDLOG_INFO("Calcite server started");
  calciteClient_->startClient();
  SPDLOG_INFO("Calcite client started");

  // execution config
  execConfig_ = ExecConfig::parseExecConfig(catalogue_, make_shared<S3Connector>(awsClient_));

  // actor system config and actor system
  const auto &remoteIps = readRemoteIps();
  actorSystemConfig_ = make_shared<ActorSystemConfig>(execConfig_->getCAFServerPort(),
                                                      remoteIps,
                                                      false);
  fpdb::executor::caf::CAFInit::initCAFGlobalMetaObjects();
  actorSystem_ = make_shared<::caf::actor_system>(*actorSystemConfig_);

  // connect to other nodes if any
  connect();

  // executor
  executor_ = make_shared<Executor>(actorSystem_,
                                    nodes_,
                                    execConfig_->getMode(),
                                    execConfig_->getCachingPolicy(),
                                    execConfig_->showOpTimes(),
                                    execConfig_->showScanMetrics());
  executor_->start();
  SPDLOG_INFO("Executor started");

  return "Client started";
}

string Client::stop() {
  // AWS client
  awsClient_->shutdown();
  SPDLOG_INFO("AWS client stopped");

  // calcite client
  calciteClient_->shutdownServer();
  SPDLOG_INFO("Calcite server stopped");

  // executor
  executor_->stop();
  SPDLOG_INFO("Executor stopped");

  return "Client stopped";
}

string Client::restart() {
  stop();
  start();
  return "Client restarted";
}

string Client::executeQuery(const string &query) {
  // fetch catalogue entry
  const auto &catalogueEntry = getCatalogueEntry(execConfig_->getSchemaName());

  // plan
  const auto &physicalPlan = plan(query, catalogueEntry);

  // execute
  const auto execRes = execute(physicalPlan);

  // output
  stringstream ss;
  ss << fmt::format("Result |\n{}", execRes.first->showString(
          TupleSetShowOptions(TupleSetShowOrientation::RowOriented)));
  ss << fmt::format("\nTime: {} secs", (double) (execRes.second) / 1000000000.0);
  ss << endl;
  return ss.str();
}

string Client::executeQueryFile(const string &queryFilePath) {
  const auto &query = readFile(queryFilePath);
  return executeQuery(query);
}

shared_ptr<CatalogueEntry> Client::getCatalogueEntry(const string &schemaName) {
  shared_ptr<CatalogueEntry> catalogueEntry;
  const auto expCatalogueEntry = catalogue_->getEntry(
          fmt::format("s3://{}/{}", execConfig_->getS3Bucket(), schemaName));
  if (expCatalogueEntry.has_value()) {
    return expCatalogueEntry.value();
  } else {
    catalogueEntry = ObjStoreCatalogueEntryReader::readCatalogueEntry(catalogue_,
                                                                      execConfig_->getS3Bucket(),
                                                                      schemaName,
                                                                      make_shared<S3Connector>(awsClient_));
    catalogue_->putEntry(catalogueEntry);
    return catalogueEntry;
  }
}

shared_ptr<PhysicalPlan> Client::plan(const string &query, const shared_ptr<CatalogueEntry> &catalogueEntry) {
  // calcite planning
  string planResult = calciteClient_->planQuery(query, execConfig_->getSchemaName());

  // deserialize plan json string into prephysical plan
  auto prePhysicalPlan = CalcitePlanJsonDeserializer::deserialize(planResult, catalogueEntry);

  // trim unused fields (Calcite trimmer does not trim completely)
  prePhysicalPlan->populateAndTrimProjectColumns();

  // transform prephysical plan to physical plan
  auto s3Connector = make_shared<obj_store::S3Connector>(awsClient_);
  auto physicalPlan = PrePToPTransformer::transform(prePhysicalPlan,
                                                    catalogueEntry,
                                                    s3Connector,
                                                    execConfig_->getMode(),
                                                    execConfig_->getParallelDegree(),
                                                    nodes_.size() + 1);

  return physicalPlan;
}

pair<shared_ptr<TupleSet>, long> Client::execute(const shared_ptr<PhysicalPlan> &physicalPlan) {
  return executor_->execute(0 /*FIXME*/, physicalPlan, execConfig_->isDistributed());
}

void Client::connect() {
  if (actorSystemConfig_->nodeIps_.empty()) {
    SPDLOG_INFO("No other nodes found, client runs in the single-node version");
  } else {
    for (const auto &nodeIp: actorSystemConfig_->nodeIps_) {
      auto expectedNode = actorSystem_->middleman().connect(nodeIp, actorSystemConfig_->port_);
      if (!expectedNode) {
        nodes_.clear();
        throw runtime_error(
                fmt::format("Failed to connected to server {}: {}", nodeIp, to_string(expectedNode.error())));
      }
      SPDLOG_INFO("Connected to server {}", nodeIp);
      nodes_.emplace_back(*expectedNode);
    }
    SPDLOG_INFO("Connected to all servers, clients runs in the distributed version");
  }
}

}

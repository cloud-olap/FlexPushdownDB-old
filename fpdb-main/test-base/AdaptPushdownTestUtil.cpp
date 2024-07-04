//
// Created by Yifei Yang on 12/13/22.
//

#include "AdaptPushdownTestUtil.h"
#include "TestUtil.h"
#include <fpdb/main/ExecConfig.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/flight/FlightClients.h>
#include <fpdb/executor/flight/FlightHandler.h>
#include <fpdb/store/server/flight/Util.hpp>
#include <fpdb/store/server/flight/adaptive/ClearAdaptPushdownMetricsCmd.hpp>
#include <fpdb/store/server/flight/adaptive/SetAdaptPushdownCmd.hpp>
#include <fpdb/store/client/FPDBStoreClientConfig.h>
#include <fpdb/util/Color.h>
#include <arrow/flight/api.h>
#include <doctest/doctest.h>
#include "thread"

namespace fpdb::main::test {

void AdaptPushdownTestUtil::run_adapt_pushdown_benchmark_query(const std::string &schemaName,
                                                               const std::vector<std::string> &queryFileNames,
                                                               const std::vector<int> &maxThreadsVec,
                                                               int parallelDegree,
                                                               bool startFPDBStore,
                                                               bool useHeuristicJoinOrdering) {
  // start daemon flight server if pushback double-exec is enabled
  std::future<tl::expected<void, std::basic_string<char>>> flight_future;
  if (store::server::flight::EnablePushbackTailReqDoubleExec) {
    auto res = executor::flight::FlightHandler::startDaemonFlightServer(
            main::ExecConfig::parseFlightPort(), flight_future);
    if (!res.has_value()) {
      throw std::runtime_error(res.error());
    }
  }

  // save old configs
  bool oldEnableAdaptPushdown = fpdb::executor::physical::ENABLE_ADAPTIVE_PUSHDOWN;
  int oldMaxThreads = fpdb::store::server::flight::MaxThreads;

  // start fpdb-store if local
  if (startFPDBStore) {
    TestUtil::startFPDBStoreServer();
  }

  // disable join ordering for TPC-H Q5
  auto startRunFunc = useHeuristicJoinOrdering ?
          TestUtil::e2eNoStartCalciteServer :
          TestUtil::e2eNoStartCalciteServerNoHeuristicJoinOrdering;

  // run pullup and pushdown once as gandiva cache makes the subsequent runs faster than the first run
  std::cout << GREEN << "Start run (pullup)" << RESET << std::endl;
  REQUIRE(startRunFunc(schemaName,
                       queryFileNames,
                       parallelDegree,
                       false,
                       ObjStoreType::FPDB_STORE,
                       Mode::pullupMode(),
                       CachingPolicyType::NONE,
                       1L * 1024 * 1024 * 1024));
  std::cout << GREEN << "Start run (pushdown)" << RESET << std::endl;
  REQUIRE(startRunFunc(schemaName,
                       queryFileNames,
                       parallelDegree,
                       false,
                       ObjStoreType::FPDB_STORE,
                       Mode::pushdownOnlyMode(),
                       CachingPolicyType::NONE,
                       1L * 1024 * 1024 * 1024));

  // collect adaptive pushdown metrics for pullup
  std::cout << GREEN << "Collect metrics run (pullup)" << RESET << std::endl;
  TestUtil testUtil(schemaName,
                    queryFileNames,
                    parallelDegree,
                    false,
                    ObjStoreType::FPDB_STORE,
                    Mode::pullupMode());
  testUtil.setCollAdaptPushdownMetrics(true);
  testUtil.setUseHeuristicJoinOrdering(useHeuristicJoinOrdering);
  REQUIRE_NOTHROW(testUtil.runTest());

  // collect adaptive pushdown metrics for pushdown
  std::cout << GREEN << "Collect metrics run (pushdown)" << RESET << std::endl;
  testUtil = TestUtil(schemaName,
                      queryFileNames,
                      parallelDegree,
                      false,
                      ObjStoreType::FPDB_STORE,
                      Mode::pushdownOnlyMode());
  testUtil.setCollAdaptPushdownMetrics(true);
  testUtil.setUseHeuristicJoinOrdering(useHeuristicJoinOrdering);
  REQUIRE_NOTHROW(testUtil.runTest());

  // measurement runs
  for (int maxThreads: maxThreadsVec) {
    std::cout << BLUE << fmt::format("Max threads at storage side: {}\n", maxThreads) << RESET << std::endl;

    // pullup baseline run
    std::cout << GREEN << "Pullup baseline run" << RESET << std::endl;
    testUtil = TestUtil(schemaName,
                        queryFileNames,
                        parallelDegree,
                        false,
                        ObjStoreType::FPDB_STORE,
                        Mode::pullupMode());
    testUtil.setUseHeuristicJoinOrdering(useHeuristicJoinOrdering);
    if (queryFileNames.size() > 1) {
      testUtil.setConcurrent(true);
    }
    set_pushdown_flags(false, maxThreads, !startFPDBStore);
    std::this_thread::sleep_for(1s);
    REQUIRE_NOTHROW(testUtil.runTest());

    // pushdown baseline run
    std::cout << GREEN << "Pushdown baseline run" << RESET << std::endl;
    testUtil = TestUtil(schemaName,
                        queryFileNames,
                        parallelDegree,
                        false,
                        ObjStoreType::FPDB_STORE,
                        Mode::pushdownOnlyMode());
    testUtil.setUseHeuristicJoinOrdering(useHeuristicJoinOrdering);
    if (queryFileNames.size() > 1) {
      testUtil.setConcurrent(true);
    }
    std::this_thread::sleep_for(1s);
    REQUIRE_NOTHROW(testUtil.runTest());

    // adaptive pushdown test run
    std::cout << GREEN << "Adaptive pushdown run" << RESET << std::endl;
    testUtil = TestUtil(schemaName,
                        queryFileNames,
                        parallelDegree,
                        false,
                        ObjStoreType::FPDB_STORE,
                        Mode::pushdownOnlyMode());
    testUtil.setUseHeuristicJoinOrdering(useHeuristicJoinOrdering);
    if (queryFileNames.size() > 1) {
      testUtil.setConcurrent(true);
    }
    set_pushdown_flags(true, maxThreads, !startFPDBStore);
    std::this_thread::sleep_for(1s);
    REQUIRE_NOTHROW(testUtil.runTest());
  }

  // restore flags and clear adaptive pushdown metrics
  set_pushdown_flags(oldEnableAdaptPushdown, oldMaxThreads, !startFPDBStore);
  send_cmd_to_storage(fpdb::store::server::flight::ClearAdaptPushdownMetricsCmd::make());

  // stop fpdb-store if local
  if (startFPDBStore) {
    TestUtil::stopFPDBStoreServer();
  }

  // stop daemon flight server if pushback double-exec is enabled
  if (store::server::flight::EnablePushbackTailReqDoubleExec) {
    executor::flight::FlightHandler::stopDaemonFlightServer(flight_future);
  }
}

void AdaptPushdownTestUtil::set_pushdown_flags(bool enableAdaptPushdown, int maxThreads,
                                               bool isFPDBStoreRemote) {
  fpdb::executor::physical::ENABLE_ADAPTIVE_PUSHDOWN = enableAdaptPushdown;
  fpdb::store::server::flight::MaxThreads = maxThreads;
  if (isFPDBStoreRemote) {
    send_cmd_to_storage(fpdb::store::server::flight::SetAdaptPushdownCmd::make(
            enableAdaptPushdown, maxThreads));
  }
}

void AdaptPushdownTestUtil::send_cmd_to_storage(const std::shared_ptr<fpdb::store::server::flight::CmdObject> &cmdObj) {
  auto expCmd = cmdObj->serialize(false);
  if (!expCmd.has_value()) {
    throw std::runtime_error(expCmd.error());
    return;
  }

  // send to each fpdb-store node
  auto fpdbStoreClientConfig = fpdb::store::client::FPDBStoreClientConfig::parseFPDBStoreClientConfig();
  for (const auto &host: fpdbStoreClientConfig->getHosts()) {
    auto client = flight::GlobalFlightClients.getFlightClient(host, fpdbStoreClientConfig->getFlightPort());
    auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
    std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
    std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
    auto status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
      return;
    }
    status = writer->Close();
    if (!status.ok()) {
      throw std::runtime_error(status.message());
      return;
    }
  }
}

}

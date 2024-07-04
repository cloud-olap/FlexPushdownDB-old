//
// Created by Yifei Yang on 3/4/22.
//

#include <doctest/doctest.h>
#include <fpdb/store/server/Server.hpp>
#include <fpdb/store/client/FPDBStoreClientConfig.h>
#include <fpdb/executor/physical/Globals.h>
#include "TestUtil.h"
#include "AdaptPushdownTestUtil.h"
#include "Globals.h"

/**
 * TPCH test (single compute node, also as the single FPDB store node)
 *
 * Start Calcite server before running this
 */
namespace fpdb::main::test {

#define SKIP_SUITE false

TEST_SUITE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-01" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/01.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-02" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/02.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-03" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/03.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-05" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/05.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-06" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/06.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-07" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/07.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-08" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/08.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-09" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/09.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-10" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/10.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-11" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/11.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-12" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/12.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-13" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/13.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-14" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/14.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-15" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/15.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-16" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/16.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-17" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/17.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-18" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/18.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-19" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/19.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-20" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/20.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-21" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/21.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pullup-22" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/22.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

}

TEST_SUITE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-01" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/01.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-02" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/02.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-03" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/03.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-05" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/05.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-06" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/06.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-07" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/07.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-08" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/08.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-09" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/09.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-10" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/10.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-11" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/11.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-12" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/12.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-13" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/13.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-14" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/14.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-15" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/15.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-16" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/16.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-17" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/17.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-18" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/18.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-19" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/19.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-20" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/20.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-21" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/21.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-csv-pushdown-only-22" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/csv/",
                                            {"tpch/original/22.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

}

TEST_SUITE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-01" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/01.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-02" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/02.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-03" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/03.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-05" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/05.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-06" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/06.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-07" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/07.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-08" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/08.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-09" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/09.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-10" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/10.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-11" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/11.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-12" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/12.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-13" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/13.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-14" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/14.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-15" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/15.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-16" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/16.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-17" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/17.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-18" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/18.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-19" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/19.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-20" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/20.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-21" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/21.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-22" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/22.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-multi-query" * doctest::skip(false || SKIP_SUITE)) {
  std::vector<std::string> queryFileNames(10, "tpch/original/02.sql");

  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            queryFileNames,
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pullup-concurrent" * doctest::skip(false || SKIP_SUITE)) {
  std::vector<std::string> queryFileNames{"tpch/original/01.sql", "tpch/original/02.sql"};

  TestUtil::startFPDBStoreServer();
  TestUtil testUtil("tpch-sf0.01/parquet/",
                    queryFileNames,
                    PARALLEL_FPDB_STORE_SAME_NODE,
                    false,
                    ObjStoreType::FPDB_STORE);
  testUtil.setConcurrent(true);
  REQUIRE_NOTHROW(testUtil.runTest());
  TestUtil::stopFPDBStoreServer();
}

}

TEST_SUITE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-01" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/01.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-02" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/02.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-03" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/03.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-05" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/05.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-06" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/06.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-07" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/07.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-08" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/08.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-09" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/09.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-10" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/10.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-11" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/11.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-12" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/12.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-13" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/13.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-14" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/14.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-15" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/15.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-16" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/16.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-17" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/17.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-18" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/18.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-19" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/19.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-20" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/20.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-21" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/21.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-parquet-pushdown-only-22" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01/parquet/",
                                            {"tpch/original/22.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

}

TEST_SUITE ("tpch-sf0.01-1-node-hash-part-hash-join-pushdown" * doctest::skip(SKIP_SUITE)) {

// Enable co-located hash join pushdown first
// for 'tpch-sf0.01-1-node-hash-part', 'lineitem' and 'orders' are co-located at 'l_orderkey = o.o_orderkey'
TEST_CASE ("tpch-sf0.01-1-node-hash-part-hash-join-pushable-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01-1-node-hash-part/parquet/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

// Enable co-located hash join pushdown first
// for 'tpch-sf0.01-1-node-hash-part', 'lineitem' and 'orders' are co-located at 'l_orderkey = o.o_orderkey'
TEST_CASE ("tpch-sf0.01-1-node-hash-part-hash-join-not-pushable-19" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01-1-node-hash-part/parquet/",
                                            {"tpch/original/19.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

// Enable co-located hash join pushdown first
// for 'tpch-sf0.01-1-node-hash-part', 'lineitem' and 'orders' are co-located at 'l_orderkey = o.o_orderkey'
// just for a pullup baseline to check the hash-join pullup part from "PrePToFPDBStorePTransformer" is correct
TEST_CASE ("tpch-sf0.01-1-node-hash-part-hash-join-pullup-04" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01-1-node-hash-part/parquet/",
                                            {"tpch/original/04.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE));
  TestUtil::stopFPDBStoreServer();
}

// Enable co-located hash join pushdown first
// for 'tpch-sf0.01-1-node-hash-part', 'lineitem' and 'orders' are co-located at 'l_orderkey = o.o_orderkey'
TEST_CASE ("tpch-sf0.01-1-node-hash-part-hash-join-synthetic-2-table" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01-1-node-hash-part/parquet/",
                                            {"tpch/synthetic/co-join-2-table.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

// Enable co-located hash join pushdown first
// for 'tpch-sf0.01-1-node-hash-part', 'lineitem' and 'orders' are co-located at 'l_orderkey = o.o_orderkey'
TEST_CASE ("tpch-sf0.01-1-node-hash-part-hash-join-synthetic-3-table" * doctest::skip(false || SKIP_SUITE)) {
  TestUtil::startFPDBStoreServer();
  REQUIRE(TestUtil::e2eNoStartCalciteServer("tpch-sf0.01-1-node-hash-part/parquet/",
                                            {"tpch/synthetic/co-join-3-table.sql"},
                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                            false,
                                            ObjStoreType::FPDB_STORE,
                                            Mode::pushdownOnlyMode()));
  TestUtil::stopFPDBStoreServer();
}

}

/**
 * Need to know the number of CPU cores at storage side, here we assume to be std::thread::hardware_concurrency()
 * as we are using the local machine.
 */
TEST_SUITE ("tpch-sf0.01-fpdb-store-same-node-adaptive-pushdown" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-adaptive-pushdown-19" * doctest::skip(false || SKIP_SUITE)) {
  AdaptPushdownTestUtil::run_adapt_pushdown_benchmark_query("tpch-sf0.01/parquet/",
                                                            {"tpch/original/19.sql"},
                                                            {(int) std::thread::hardware_concurrency(), 1},
                                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                                            true);
}

TEST_CASE ("tpch-sf0.01-fpdb-store-same-node-adaptive-pushdown-concurrent" * doctest::skip(false || SKIP_SUITE)) {
  AdaptPushdownTestUtil::run_adapt_pushdown_benchmark_query("tpch-sf0.01/parquet/",
                                                            {"tpch/original/01.sql", "tpch/original/02.sql"},
                                                            {(int) std::thread::hardware_concurrency(), 1},
                                                            PARALLEL_FPDB_STORE_SAME_NODE,
                                                            true);
}

}

}

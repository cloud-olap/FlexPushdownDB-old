//
// Created by Yifei Yang on 12/6/22.
//

#include "BitmapPushdownTestUtil.h"
#include "TestUtil.h"
#include "Globals.h"
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/plan/prephysical/separable/Globals.h>
#include <doctest/doctest.h>
#include <fmt/format.h>

namespace fpdb::main::test {

void BitmapPushdownTestUtil::run_bitmap_pushdown_benchmark_query(const std::string &cachingQuery,
                                                                 const std::vector<std::string> &testQueries,
                                                                 const std::vector<std::string> &testQueryFileNames,
                                                                 bool isSsb,
                                                                 const std::string &sf,
                                                                 int parallelDegree,
                                                                 bool startFPDBStore) {
  const std::string cachingQueryFileName = "caching.sql";

  // write query to file and make fix layout indices
  std::set<int> fixLayoutIndices;
  TestUtil::writeQueryToFile(cachingQueryFileName, cachingQuery);
  for (size_t i = 0; i < testQueryFileNames.size(); ++i) {
    TestUtil::writeQueryToFile(testQueryFileNames[i], testQueries[i]);
    fixLayoutIndices.emplace(i);
  }

  // combine caching query and test queries
  std::vector<std::string> queryFileNames{cachingQueryFileName};
  queryFileNames.insert(queryFileNames.end(), testQueryFileNames.begin(), testQueryFileNames.end());

  // test
  if (startFPDBStore) {
    TestUtil::startFPDBStoreServer();
  }
  TestUtil testUtil(isSsb ? fmt::format("ssb-sf{}/parquet/", sf) : fmt::format("tpch-sf{}/parquet/", sf),
                    queryFileNames,
                    parallelDegree,
                    false,
                    ObjStoreType::FPDB_STORE,
                    Mode::hybridMode(),
                    CachingPolicyType::LFU,
                    std::numeric_limits<int64_t>::max());

  // enable filter bitmap pushdown
  std::unordered_map<std::string, bool> flags;
  set_pushdown_flags(flags);

  // fix cache layout after caching query, otherwise it keeps fetching new segments
  // which is not intra-partition hybrid execution (i.e. hit data + loaded new cache data is enough for execution,
  // pushdown part actually does nothing)
  testUtil.setFixLayoutIndices(fixLayoutIndices);

  REQUIRE_NOTHROW(testUtil.runTest());
  if (startFPDBStore) {
    TestUtil::stopFPDBStoreServer();
  }

  // reset pushdown flags
  reset_pushdown_flags(flags);

  // clear query file
  TestUtil::removeQueryFile(cachingQueryFileName);
  for (size_t i = 0; i < testQueryFileNames.size(); ++i) {
    TestUtil::removeQueryFile(testQueryFileNames[i]);
  }
}

void BitmapPushdownTestUtil::set_pushdown_flags(std::unordered_map<std::string, bool> &flags) {
  flags["group"] = fpdb::executor::physical::ENABLE_GROUP_BY_PUSHDOWN;
  flags["bloom_filter"] = fpdb::executor::physical::ENABLE_BLOOM_FILTER_PUSHDOWN;
  flags["shuffle"] = fpdb::executor::physical::ENABLE_SHUFFLE_PUSHDOWN;
  flags["co_located_join"] = fpdb::plan::prephysical::separable::ENABLE_CO_LOCATED_JOIN_PUSHDOWN;
  flags["filter_bitmap"] = fpdb::executor::physical::ENABLE_FILTER_BITMAP_PUSHDOWN;
  fpdb::executor::physical::ENABLE_GROUP_BY_PUSHDOWN = false;
  fpdb::executor::physical::ENABLE_BLOOM_FILTER_PUSHDOWN = false;
  fpdb::executor::physical::ENABLE_SHUFFLE_PUSHDOWN = false;
  fpdb::plan::prephysical::separable::ENABLE_CO_LOCATED_JOIN_PUSHDOWN = false;
  fpdb::executor::physical::ENABLE_FILTER_BITMAP_PUSHDOWN = true;
}

void BitmapPushdownTestUtil::reset_pushdown_flags(std::unordered_map<std::string, bool> &flags) {
  fpdb::executor::physical::ENABLE_GROUP_BY_PUSHDOWN = flags["group"];
  fpdb::executor::physical::ENABLE_BLOOM_FILTER_PUSHDOWN = flags["bloom_filter"];
  fpdb::executor::physical::ENABLE_SHUFFLE_PUSHDOWN = flags["shuffle"];
  fpdb::plan::prephysical::separable::ENABLE_CO_LOCATED_JOIN_PUSHDOWN = flags["co_located_join"];
  fpdb::executor::physical::ENABLE_FILTER_BITMAP_PUSHDOWN = flags["filter_bitmap"];
}

}

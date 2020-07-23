//
// Created by matt on 23/7/20.
//

#include <memory>
#include <experimental/filesystem>

#include <doctest/doctest.h>

#include <normal/pushdown/Collate.h>
#include <normal/core/OperatorManager.h>
#include <normal/core/graph/OperatorGraph.h>
#include <normal/pushdown/file/FileScan.h>
#include <normal/tuple/TupleSet2.h>
#include <normal/pushdown/aggregate/Sum.h>
#include <normal/expression/gandiva/Column.h>
#include <normal/core/type/Float64Type.h>
#include <normal/expression/gandiva/Cast.h>
#include <normal/pushdown/Aggregate.h>
#include <normal/ssb/TestUtil.h>
#include <normal/pushdown/Project.h>

using namespace normal::pushdown;
using namespace normal::pushdown::aggregate;
using namespace normal::tuple;
using namespace normal::core::type;
using namespace normal::core::graph;
using namespace normal::expression;
using namespace normal::expression::gandiva;
using namespace std::experimental;
using namespace normal::ssb;

#define SKIP_SUITE false

TEST_SUITE ("aggregate" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("sum" * doctest::skip(false || SKIP_SUITE)) {

  auto aFile = filesystem::absolute("data/ssb-sf1/lineorder.tbl");
  auto numBytesAFile = filesystem::file_size(aFile);

  auto mgr = std::make_shared<OperatorManager>();
  mgr->boot();
  mgr->start();

  auto g = OperatorGraph::make(mgr);

  auto fileScan = FileScan::make("fileScan",
								 aFile,
								 std::vector<std::string>{"LO_EXTENDEDPRICE"},
								 0,
								 numBytesAFile,
								 g->getId(),
								 true);
  auto project = std::make_shared<Project>("project", std::vector<std::shared_ptr<normal::expression::gandiva::Expression>>{col("LO_EXTENDEDPRICE")});
  auto aggregateFunctions = std::make_shared<std::vector<std::shared_ptr<AggregationFunction>>>();
  aggregateFunctions->
	  emplace_back(std::make_shared<Sum>("sum", cast(col("LO_EXTENDEDPRICE"), float64Type()))
  );
  auto aggregate = std::make_shared<Aggregate>("aggregate", aggregateFunctions);
  auto collate = std::make_shared<Collate>("collate", g->getId());

  fileScan->produce(project);
  project->consume(fileScan);

  project->produce(aggregate);
  aggregate->consume(project);

  aggregate->produce(collate);
  collate->consume(aggregate);

  g->put(fileScan);
  g->put(project);
  g->put(aggregate);
  g->put(collate);

  TestUtil::writeExecutionPlan2(*g);

  g->boot();

  g->start();
  g->join();

  auto tuples = collate->tuples();

  auto val = tuples->value<arrow::DoubleType>("Sum", 0);

	  CHECK(tuples->numRows() == 1);
	  CHECK(tuples->numColumns() == 1);
	  CHECK_EQ(33.0, val.value());

  mgr->stop();

}

}
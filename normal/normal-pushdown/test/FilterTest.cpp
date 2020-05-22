//
// Created by matt on 6/5/20.
//


#include <memory>

#include <doctest/doctest.h>

#include <normal/pushdown/Collate.h>
#include <normal/core/OperatorManager.h>
#include <normal/pushdown/file/FileScan.h>
#include <normal/test/TestUtil.h>
#include <normal/pushdown/filter/Filter.h>
#include <normal/pushdown/filter/FilterPredicate.h>
#include <normal/expression/gandiva/Column.h>
#include <normal/expression/gandiva/Literal.h>
#include <normal/expression/gandiva/LessThan.h>

using namespace normal::pushdown;
using namespace normal::pushdown::filter;
using namespace normal::tuple;
using namespace normal::expression::gandiva;

#define SKIP_SUITE true

TEST_SUITE ("filter" * doctest::skip(SKIP_SUITE)) {

TEST_CASE ("filescan-filter-collate" * doctest::skip(false || SKIP_SUITE)) {

  auto mgr = std::make_shared<normal::core::OperatorManager>();

  auto scan = std::make_shared<FileScan>("fileScan", "data/filter/a.csv");
  auto filter = Filter::make("filter", FilterPredicate::make(lt(col("AA"), lit<::arrow::Int64Type>(11))));
  auto collate = std::make_shared<Collate>("collate");

  scan->produce(filter);
  filter->consume(scan);

  filter->produce(collate);
  collate->consume(filter);

  mgr->put(scan);
  mgr->put(filter);
  mgr->put(collate);

  normal::test::TestUtil::writeExecutionPlan(*mgr);

  mgr->boot();

  mgr->start();
  mgr->join();

  auto tuples = collate->tuples();

  mgr->stop();

  auto tupleSet = TupleSet2::create(tuples);

  SPDLOG_INFO("Output:\n{}", tupleSet->showString(TupleSetShowOptions(TupleSetShowOrientation::RowOriented)));

	  CHECK(tupleSet->numRows() == 1);
	  CHECK(tupleSet->numColumns() == 3);

  /*
   * FIXME: The following assumes the output is produced in a specific order but this shouldn't necessarily
   *  be assumed. Will only be able to check the properly once we have a sort operator
   */
  auto columnAA = tupleSet->getColumnByName("AA").value();
	  CHECK(columnAA->element(0).value()->value<long>() == 10);

  auto columnAB = tupleSet->getColumnByName("AB").value();
	  CHECK(columnAB->element(0).value()->value<long>() == 13);

  auto columnAC = tupleSet->getColumnByName("AC").value();
	  CHECK(columnAC->element(0).value()->value<long>() == 16);

}

}
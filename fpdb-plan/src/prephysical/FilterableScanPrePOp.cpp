//
// Created by Yifei Yang on 11/8/21.
//

#include <fpdb/plan/prephysical/FilterableScanPrePOp.h>

namespace fpdb::plan::prephysical {

FilterableScanPrePOp::FilterableScanPrePOp(uint id, const shared_ptr<Table> &table) :
  PrePhysicalOp(id, FILTERABLE_SCAN),
  table_(table) {}

string FilterableScanPrePOp::getTypeString() {
  return "FilterableScanPrePOp";
}

set<string> FilterableScanPrePOp::getUsedColumnNames() {
  set<string> usedColumnNames = getProjectColumnNames();
  if (predicate_) {
    const auto &predicateColumnNames = predicate_->involvedColumnNames();
    usedColumnNames.insert(predicateColumnNames.begin(), predicateColumnNames.end());
  }
  return usedColumnNames;
}

void FilterableScanPrePOp::setProjectColumnNames(const set<string> &projectColumnNames) {
  // scan operator shouldn't scan no columns
  if (projectColumnNames.empty()) {
    PrePhysicalOp::setProjectColumnNames({table_->getColumnNames()[0]});
  } else {
    PrePhysicalOp::setProjectColumnNames(projectColumnNames);
  }
}

const shared_ptr<fpdb::expression::gandiva::Expression> &FilterableScanPrePOp::getPredicate() const {
  return predicate_;
}

const shared_ptr<Table> &FilterableScanPrePOp::getTable() const {
  return table_;
}

void FilterableScanPrePOp::setPredicate(const shared_ptr<fpdb::expression::gandiva::Expression> &predicate) {
  predicate_ = predicate;
}

}

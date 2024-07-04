//
// Created by Yifei Yang on 11/8/21.
//

#include <fpdb/plan/prephysical/FilterPrePOp.h>

namespace fpdb::plan::prephysical {

FilterPrePOp::FilterPrePOp(uint id, const shared_ptr<fpdb::expression::gandiva::Expression> &predicate) :
  PrePhysicalOp(id, FILTER),
  predicate_(predicate) {}

string FilterPrePOp::getTypeString() {
  return "FilterPrePOp";
}

set<string> FilterPrePOp::getUsedColumnNames() {
  set<string> usedColumnNames = getProjectColumnNames();
  const auto &predicateColumnNames = predicate_->involvedColumnNames();
  usedColumnNames.insert(predicateColumnNames.begin(), predicateColumnNames.end());
  return usedColumnNames;
}

const shared_ptr<fpdb::expression::gandiva::Expression> &FilterPrePOp::getPredicate() const {
  return predicate_;
}

}

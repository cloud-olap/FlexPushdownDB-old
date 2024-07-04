//
// Created by Yifei Yang on 11/1/21.
//

#include <fpdb/plan/prephysical/AggregatePrePOp.h>

namespace fpdb::plan::prephysical {

AggregatePrePOp::AggregatePrePOp(uint id,
                                 const vector<string> &aggOutputColumnNames,
                                 const vector<shared_ptr<AggregatePrePFunction>> &functions):
  PrePhysicalOp(id, AGGREGATE),
  aggOutputColumnNames_(aggOutputColumnNames),
  functions_(functions) {}

const vector<string> &AggregatePrePOp::getAggOutputColumnNames() const {
  return aggOutputColumnNames_;
}

const vector<shared_ptr<AggregatePrePFunction>> &AggregatePrePOp::getFunctions() const {
  return functions_;
}

string AggregatePrePOp::getTypeString() {
  return "AggregatePrePOp";
}

set<string> AggregatePrePOp::getUsedColumnNames() {
  set<string> usedColumnNames;
  for (const auto &function: functions_) {
    const auto involvedColumnNames = function->involvedColumnNames();
    usedColumnNames.insert(involvedColumnNames.begin(), involvedColumnNames.end());
  }
  return usedColumnNames;
}

}
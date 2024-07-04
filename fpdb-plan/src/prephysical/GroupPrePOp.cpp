//
// Created by Yifei Yang on 11/1/21.
//

#include <fpdb/plan/prephysical/GroupPrePOp.h>

namespace fpdb::plan::prephysical {

GroupPrePOp::GroupPrePOp(uint id,
                         const vector<string> &groupColumnNames,
                         const vector<string> &aggOutputColumnNames,
                         const vector<shared_ptr<AggregatePrePFunction>> &functions) :
   PrePhysicalOp(id, GROUP),
   groupColumnNames_(groupColumnNames),
   aggOutputColumnNames_(aggOutputColumnNames),
   functions_(functions) {}

string GroupPrePOp::getTypeString() {
  return "GroupPrePOp";
}

set<string> GroupPrePOp::getUsedColumnNames() {
  set<string> usedColumnNames(groupColumnNames_.begin(), groupColumnNames_.end());
  for (const auto &function: functions_) {
    const auto involvedColumnNames = function->involvedColumnNames();
    usedColumnNames.insert(involvedColumnNames.begin(), involvedColumnNames.end());
  }
  return usedColumnNames;
}

const vector<string> &GroupPrePOp::getGroupColumnNames() const {
  return groupColumnNames_;
}

const vector<string> &GroupPrePOp::getAggOutputColumnNames() const {
  return aggOutputColumnNames_;
}

const vector<shared_ptr<AggregatePrePFunction>> &GroupPrePOp::getFunctions() const {
  return functions_;
}

}

//
// Created by Yifei Yang on 12/6/21.
//

#include <fpdb/plan/prephysical/LimitSortPrePOp.h>
#include <fpdb/plan/prephysical/PrePOpType.h>

namespace fpdb::plan::prephysical {

LimitSortPrePOp::LimitSortPrePOp(uint id,
                                 int64_t k,
                                 const vector<SortKey> &sortKeys):
  PrePhysicalOp(id, LIMIT_SORT),
  k_(k),
  sortKeys_(sortKeys) {}

string LimitSortPrePOp::getTypeString() {
  return "LimitSortPrePOp";
}

set<string> LimitSortPrePOp::getUsedColumnNames() {
  auto usedColumnNames = getProjectColumnNames();
  for (const auto &sortKey: sortKeys_) {
    usedColumnNames.emplace(sortKey.getName());
  }
  return usedColumnNames;
}

int64_t LimitSortPrePOp::getK() const {
  return k_;
}

const vector<SortKey> &LimitSortPrePOp::getSortKeys() const {
  return sortKeys_;
}

}

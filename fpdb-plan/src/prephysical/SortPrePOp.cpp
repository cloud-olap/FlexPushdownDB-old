//
// Created by Yifei Yang on 10/31/21.
//

#include <fpdb/plan/prephysical/SortPrePOp.h>
#include <fpdb/plan/prephysical/PrePOpType.h>

using namespace std;

namespace fpdb::plan::prephysical {

SortPrePOp::SortPrePOp(uint id, const vector<SortKey> &sortKeys) :
  PrePhysicalOp(id, SORT),
  sortKeys_(sortKeys) {}

string SortPrePOp::getTypeString() {
  return "SortPrePOp";
}

set<string> SortPrePOp::getUsedColumnNames() {
  auto usedColumnNames = getProjectColumnNames();
  for (const auto &sortKey: sortKeys_) {
    usedColumnNames.emplace(sortKey.getName());
  }
  return usedColumnNames;
}

const vector<SortKey> &SortPrePOp::getSortKeys() const {
  return sortKeys_;
}

}

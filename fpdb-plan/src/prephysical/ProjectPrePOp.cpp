//
// Created by Yifei Yang on 11/7/21.
//

#include <fpdb/plan/prephysical/ProjectPrePOp.h>
#include <fpdb/plan/Util.h>

using namespace fpdb::plan;

namespace fpdb::plan::prephysical {
ProjectPrePOp::ProjectPrePOp(uint id,
                             const vector<shared_ptr<fpdb::expression::gandiva::Expression>> &exprs,
                             const vector<std::string> &exprNames,
                             const vector<pair<string, string>> &projectColumnNamePairs) :
  PrePhysicalOp(id, PROJECT),
  exprs_(exprs),
  exprNames_(exprNames),
  projectColumnNamePairs_(projectColumnNamePairs) {}

string ProjectPrePOp::getTypeString() {
  return "ProjectPrePOp";
}

set<string> ProjectPrePOp::getUsedColumnNames() {
  set<string> usedColumnNames = getProjectColumnNames();
  for (const auto &columnPair: projectColumnNamePairs_) {
    usedColumnNames.emplace(columnPair.first);
  }

  for (const auto &expr: exprs_) {
    const auto involvedColumnNames = expr->involvedColumnNames();
    usedColumnNames.insert(involvedColumnNames.begin(), involvedColumnNames.end());
  }
  return usedColumnNames;
}

const vector<shared_ptr<fpdb::expression::gandiva::Expression>> &ProjectPrePOp::getExprs() const {
  return exprs_;
}

const vector<std::string> &ProjectPrePOp::getExprNames() const {
  return exprNames_;
}

const vector<pair<string, string>> &ProjectPrePOp::getProjectColumnNamePairs() const {
  return projectColumnNamePairs_;
}

void ProjectPrePOp::setProjectColumnNames(const set<string> &projectColumnNames) {
  // also need to update projectColumnNamePairs
  updateProjectColumnNamePairs(projectColumnNames);
  PrePhysicalOp::setProjectColumnNames(projectColumnNames);
}

void ProjectPrePOp::updateProjectColumnNamePairs(const set<string> &projectColumnNames) {
  // check if additional projectColumnNames are added
  set<string> columnFromNamePairsAndExprs(exprNames_.begin(), exprNames_.end());
  for (const auto &namePair: projectColumnNamePairs_) {
    columnFromNamePairsAndExprs.emplace(namePair.second);
  }
  for (const auto &columnName: projectColumnNames) {
    if (columnFromNamePairsAndExprs.find(columnName) == columnFromNamePairsAndExprs.end()) {
      projectColumnNamePairs_.emplace_back(make_pair(columnName, columnName));
    }
  }

  // check if some projectColumnNames are excluded
  for (auto it = projectColumnNamePairs_.begin(); it != projectColumnNamePairs_.end();) {
    if (projectColumnNames.find(it->second) == projectColumnNames.end()) {
      it = projectColumnNamePairs_.erase(it);
    } else {
      ++it;
    }
  }
}

}

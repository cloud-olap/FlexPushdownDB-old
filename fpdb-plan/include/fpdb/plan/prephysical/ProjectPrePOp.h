//
// Created by Yifei Yang on 11/7/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_PROJECTPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_PROJECTPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/expression/gandiva/Expression.h>

namespace fpdb::plan::prephysical {

class ProjectPrePOp: public PrePhysicalOp {
public:
  constexpr static const char *const DUMMY_COLUMN_PREFIX = "dummy";

  ProjectPrePOp(uint id,
                const vector<shared_ptr<fpdb::expression::gandiva::Expression>> &exprs,
                const vector<std::string> &exprNames,
                const vector<pair<string, string>> &projectColumnNamePairs);

  string getTypeString() override;

  set<string> getUsedColumnNames() override;
  void setProjectColumnNames(const set<string> &projectColumnNames) override;

  const vector<shared_ptr<fpdb::expression::gandiva::Expression>> &getExprs() const;
  const vector<std::string> &getExprNames() const;
  const vector<pair<string, string>> &getProjectColumnNamePairs() const;

private:
  void updateProjectColumnNamePairs(const set<string> &projectColumnNames);

  vector<shared_ptr<fpdb::expression::gandiva::Expression>> exprs_;
  vector<string> exprNames_;
  vector<pair<string, string>> projectColumnNamePairs_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_PROJECTPREPOP_H

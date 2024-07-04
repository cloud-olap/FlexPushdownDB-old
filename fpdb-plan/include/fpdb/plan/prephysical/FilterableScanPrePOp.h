//
// Created by Yifei Yang on 11/8/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERABLESCANPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERABLESCANPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/expression/gandiva/Expression.h>
#include <fpdb/catalogue/Table.h>

using namespace fpdb::catalogue;

namespace fpdb::plan::prephysical {

class FilterableScanPrePOp: public PrePhysicalOp {
public:
  FilterableScanPrePOp(uint id, const shared_ptr<Table> &table);

  string getTypeString() override;
  set<string> getUsedColumnNames() override;
  void setProjectColumnNames(const set<string> &projectColumnNames) override;

  const shared_ptr<fpdb::expression::gandiva::Expression> &getPredicate() const;
  const shared_ptr<Table> &getTable() const;

  void setPredicate(const shared_ptr<fpdb::expression::gandiva::Expression> &predicate);

private:
  shared_ptr<fpdb::expression::gandiva::Expression> predicate_;
  shared_ptr<Table> table_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERABLESCANPREPOP_H

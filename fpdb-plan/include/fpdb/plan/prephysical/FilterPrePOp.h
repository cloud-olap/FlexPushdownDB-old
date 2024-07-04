//
// Created by Yifei Yang on 11/8/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/expression/gandiva/Expression.h>

namespace fpdb::plan::prephysical {

class FilterPrePOp: public PrePhysicalOp {
public:
  FilterPrePOp(uint id, const shared_ptr<fpdb::expression::gandiva::Expression> &predicate);

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

  const shared_ptr<fpdb::expression::gandiva::Expression> &getPredicate() const;

private:
  shared_ptr<fpdb::expression::gandiva::Expression> predicate_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_FILTERPREPOP_H

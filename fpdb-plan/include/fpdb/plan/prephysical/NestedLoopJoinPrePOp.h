//
// Created by Yifei Yang on 12/14/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_NESTEDLOOPJOINPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_NESTEDLOOPJOINPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/plan/prephysical/JoinType.h>
#include <fpdb/expression/gandiva/Expression.h>

namespace fpdb::plan::prephysical {

class NestedLoopJoinPrePOp: public PrePhysicalOp {

public:
  NestedLoopJoinPrePOp(uint id,
                       JoinType joinType,
                       const shared_ptr<fpdb::expression::gandiva::Expression> &predicate);

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

  JoinType getJoinType() const;
  const shared_ptr<fpdb::expression::gandiva::Expression> &getPredicate() const;

private:
  JoinType joinType_;
  shared_ptr<fpdb::expression::gandiva::Expression> predicate_;

};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_NESTEDLOOPJOINPREPOP_H

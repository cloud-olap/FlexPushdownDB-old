//
// Created by Yifei Yang on 2/26/22.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLESUPERPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLESUPERPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>

namespace fpdb::plan::prephysical::separable {

/**
 * Denote a group of separable operators that can be pushed to store
 */
class SeparableSuperPrePOp: public PrePhysicalOp {

public:
  SeparableSuperPrePOp(uint id, const std::shared_ptr<PrePhysicalOp> &rootOp);

  const std::shared_ptr<PrePhysicalOp> &getRootOp() const;

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

private:
  std::shared_ptr<PrePhysicalOp> rootOp_;

};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_SEPARABLESUPERPREPOP_H

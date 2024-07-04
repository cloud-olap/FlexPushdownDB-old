//
// Created by Yifei Yang on 11/1/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_GROUPPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_GROUPPREPOP_H

#include <fpdb/plan/prephysical/AggregatePrePFunctionType.h>
#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/plan/prephysical/AggregatePrePFunction.h>

namespace fpdb::plan::prephysical {

class GroupPrePOp: public PrePhysicalOp {
public:
  GroupPrePOp(uint id,
              const vector<string> &groupColumnNames,
              const vector<string> &aggOutputColumnNames,
              const vector<shared_ptr<AggregatePrePFunction>> &functions);

  string getTypeString() override;
  set<string> getUsedColumnNames() override;

  const vector<string> &getGroupColumnNames() const;
  const vector<string> &getAggOutputColumnNames() const;
  const vector<shared_ptr<AggregatePrePFunction>> &getFunctions() const;

private:
  vector<string> groupColumnNames_;
  vector<string> aggOutputColumnNames_;
  vector<shared_ptr<AggregatePrePFunction>> functions_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_GROUPPREPOP_H

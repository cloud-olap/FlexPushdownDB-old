//
// Created by Yifei Yang on 11/1/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_AGGREGATEPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_AGGREGATEPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/plan/prephysical/AggregatePrePFunction.h>

namespace fpdb::plan::prephysical {

class AggregatePrePOp: public PrePhysicalOp {
public:
  AggregatePrePOp(uint id,
                  const vector<string> &aggOutputColumnNames,
                  const vector<shared_ptr<AggregatePrePFunction>> &functions);

  const vector<string> &getAggOutputColumnNames() const;
  const vector<shared_ptr<AggregatePrePFunction>> &getFunctions() const;

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

private:
  vector<string> aggOutputColumnNames_;
  vector<shared_ptr<AggregatePrePFunction>> functions_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_AGGREGATEPREPOP_H

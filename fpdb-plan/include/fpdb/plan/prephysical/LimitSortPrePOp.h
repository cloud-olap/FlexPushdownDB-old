//
// Created by Yifei Yang on 12/6/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_LIMITSORTPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_LIMITSORTPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/plan/prephysical/SortKey.h>

using namespace std;

namespace fpdb::plan::prephysical {

class LimitSortPrePOp: public PrePhysicalOp {
public:
  LimitSortPrePOp(uint id,
                  int64_t k,
                  const vector<SortKey> &sortKeys);

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

  int64_t getK() const;
  const vector<SortKey> &getSortKeys() const;

private:
  int64_t k_;
  vector<SortKey> sortKeys_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICAL_LIMITSORTPREPOP_H

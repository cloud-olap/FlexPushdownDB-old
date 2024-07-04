//
// Created by Yifei Yang on 10/31/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_SORTPREPOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_SORTPREPOP_H

#include <fpdb/plan/prephysical/PrePhysicalOp.h>
#include <fpdb/plan/prephysical/SortKey.h>

using namespace std;

namespace fpdb::plan::prephysical {

class SortPrePOp: public PrePhysicalOp {
public:
  SortPrePOp(uint id, const vector<SortKey> &sortKeys);

  string getTypeString() override;

  set<string> getUsedColumnNames() override;

  const vector<SortKey> &getSortKeys() const;

private:
  vector<SortKey> sortKeys_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_SORTPREPOP_H

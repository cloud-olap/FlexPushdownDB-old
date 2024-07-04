//
// Created by Yifei Yang on 10/31/21.
//

#ifndef FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_PREPHYSICALOP_H
#define FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_PREPHYSICALOP_H

#include <fpdb/plan/prephysical/PrePOpType.h>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <sys/types.h>

using namespace std;

namespace fpdb::plan::prephysical {

class PrePhysicalOp {
public:
  PrePhysicalOp(uint id, PrePOpType type);
  virtual ~PrePhysicalOp() = default;

  uint getId() const;
  PrePOpType getType() const;
  virtual string getTypeString() = 0;
  const vector<shared_ptr<PrePhysicalOp>> &getProducers() const;
  const set<string> &getProjectColumnNames() const;
  virtual set<string> getUsedColumnNames() = 0;

  void setProducers(const vector<shared_ptr<PrePhysicalOp>> &producers);
  virtual void setProjectColumnNames(const set<string> &projectColumnNames);

private:
  uint id_;
  PrePOpType type_;
  vector<shared_ptr<PrePhysicalOp>> producers_;
  set<string> projectColumnNames_;
};

}


#endif //FPDB_FPDB_PLAN_INCLUDE_FPDB_PLAN_PREPHYSICALOP_PREPHYSICALOP_H

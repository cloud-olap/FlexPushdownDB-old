//
// Created by Yifei Yang on 4/25/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWKERNEL_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWKERNEL_H

#include <fpdb/executor/physical/join/hashjoin/HashJoinPredicate.h>
#include <fpdb/plan/prephysical/JoinType.h>
#include <fpdb/tuple/TupleSet.h>
#include <arrow/compute/exec/options.h>
#include <set>

using namespace fpdb::plan::prephysical;
using namespace fpdb::tuple;

namespace fpdb::executor::physical::join {

struct HashJoinArrowExecPlanSuite {
  std::shared_ptr<arrow::compute::ExecContext> execContext_;
  std::shared_ptr<arrow::compute::ExecPlan> execPlan_;
  arrow::compute::ExecNode* buildInputNode_;
  arrow::compute::ExecNode* probeInputNode_;
  arrow::compute::ExecNode* hashJoinNode_;
  arrow::compute::ExecNode* bufferedSinkNode_;
  int numBuildInputBatches_;
  int numProbeInputBatches_;
};

class HashJoinArrowKernel {

public:
  HashJoinArrowKernel(const HashJoinPredicate &pred,
                      const std::set<std::string> &neededColumnNames,
                      JoinType joinType);
  HashJoinArrowKernel() = default;
  HashJoinArrowKernel(const HashJoinArrowKernel&) = default;
  HashJoinArrowKernel& operator=(const HashJoinArrowKernel&) = default;
  ~HashJoinArrowKernel() = default;

  static HashJoinArrowKernel make(const HashJoinPredicate &pred,
                                  const std::set<std::string> &neededColumnNames,
                                  JoinType joinType);

  const HashJoinPredicate &getPred() const;
  JoinType getJoinType() const;

  tl::expected<void, std::string> joinBuildTupleSet(const std::shared_ptr<TupleSet> &tupleSet);
  tl::expected<void, std::string> joinProbeTupleSet(const std::shared_ptr<TupleSet> &tupleSet);
  void finalizeInput(bool isBuildSide);

  const std::optional<std::shared_ptr<arrow::Schema>> &getOutputSchema() const;
  const std::optional<std::shared_ptr<TupleSet>> &getOutputBuffer() const;
  void clearOutputBuffer();

  void clear();

private:
  tl::expected<void, std::string> makeOutputSchema();
  tl::expected<void, std::string> makeArrowExecPlan();
  tl::expected<void, std::string> consumeInput(const std::shared_ptr<TupleSet> &tupleSet, bool isBuildSide);
  tl::expected<void, std::string> doFinalizeInput(bool isBuildSide);
  tl::expected<void, std::string> bufferInput(const std::shared_ptr<TupleSet> &tupleSet, bool isBuildSide);
  tl::expected<void, std::string> bufferOutput(const std::shared_ptr<TupleSet> &tupleSet);

  HashJoinPredicate pred_;
  std::set<std::string> neededColumnNames_;
  JoinType joinType_;

  std::optional<std::shared_ptr<arrow::Schema>> buildInputSchema_;
  std::optional<std::shared_ptr<arrow::Schema>> probeInputSchema_;
  std::optional<std::shared_ptr<arrow::Schema>> outputSchema_;
  std::optional<arrow::compute::HashJoinNodeOptions> hashJoinNodeOptions_;
  std::optional<HashJoinArrowExecPlanSuite> arrowExecPlanSuite_;
  std::optional<std::shared_ptr<TupleSet>> buildInputBuffer_;
  std::optional<std::shared_ptr<TupleSet>> probeInputBuffer_;
  std::optional<std::shared_ptr<TupleSet>> outputBuffer_;
  bool buildInputFinalized_ = false;
  bool probeInputFinalized_ = false;

// caf inspect
public:
  template <class Inspector>
  friend bool inspect(Inspector& f, HashJoinArrowKernel& kernel) {
    return f.object(kernel).fields(f.field("pred", kernel.pred_),
                                   f.field("neededColumnNames", kernel.neededColumnNames_),
                                   f.field("joinType", kernel.joinType_));
  }
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_JOIN_HASHJOIN_HASHJOINARROWKERNEL_H

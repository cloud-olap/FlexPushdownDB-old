//
// Created by Yifei Yang on 4/25/22.
//

#include <fpdb/executor/physical/join/hashjoin/HashJoinArrowKernel.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/tuple/arrow/exec/DummyNode.h>
#include <fpdb/tuple/arrow/exec/BufferedSinkNode.h>
#include <fpdb/tuple/util/Util.h>
#include <arrow/compute/exec/exec_plan.h>

namespace fpdb::executor::physical::join {

HashJoinArrowKernel::HashJoinArrowKernel(const HashJoinPredicate &pred,
                                         const std::set<std::string> &neededColumnNames,
                                         JoinType joinType):
  pred_(pred),
  neededColumnNames_(neededColumnNames),
  joinType_(joinType) {}

HashJoinArrowKernel HashJoinArrowKernel::make(const HashJoinPredicate &pred,
                                              const std::set<std::string> &neededColumnNames,
                                              JoinType joinType) {
  return {pred, neededColumnNames, joinType};
}

const HashJoinPredicate &HashJoinArrowKernel::getPred() const {
  return pred_;
}

JoinType HashJoinArrowKernel::getJoinType() const {
  return joinType_;
}

tl::expected<void, std::string> HashJoinArrowKernel::joinBuildTupleSet(const std::shared_ptr<TupleSet> &tupleSet) {
  // buffer input schema and make output schema
  if (!buildInputSchema_.has_value()) {
    buildInputSchema_ = tupleSet->schema();
  }
  auto res = makeOutputSchema();
  if (!res.has_value()) {
    return res;
  }

  // skip empty tupleSet
  if (tupleSet->numRows() == 0) {
    return {};
  }

  // if arrow exec plan is made, just put input into it
  if (arrowExecPlanSuite_.has_value()) {
    return consumeInput(tupleSet, true);
  }

  // otherwise, buffer input and make arrow exec plan
  bufferInput(tupleSet, true);
  res = makeArrowExecPlan();
  if (!res.has_value()) {
    return res;
  }

  return {};
}

tl::expected<void, std::string> HashJoinArrowKernel::joinProbeTupleSet(const std::shared_ptr<TupleSet> &tupleSet) {
  // buffer input schema and make output schema
  if (!probeInputSchema_.has_value()) {
    probeInputSchema_ = tupleSet->schema();
  }
  auto res = makeOutputSchema();
  if (!res.has_value()) {
    return res;
  }

  // skip empty tupleSet
  if (tupleSet->numRows() == 0) {
    return {};
  }

  // if arrow exec plan is made, just put input into it
  if (arrowExecPlanSuite_.has_value()) {
    return consumeInput(tupleSet, false);
  }

  // otherwise, buffer input and make arrow exec plan
  bufferInput(tupleSet, false);
  res = makeArrowExecPlan();
  if (!res.has_value()) {
    return res;
  }

  return {};
}

void HashJoinArrowKernel::finalizeInput(bool isBuildSide) {
  // make arrow exec plan if not yet, this may occur if only one side has input for outer joins
  if (!arrowExecPlanSuite_.has_value()) {
    makeArrowExecPlan();
  }

  // do finalize
  if (arrowExecPlanSuite_.has_value()) {
    doFinalizeInput(isBuildSide);
  }

  // mark flag
  if (isBuildSide) {
    buildInputFinalized_ = true;
  } else {
    probeInputFinalized_ = true;
  }
}

const std::optional<std::shared_ptr<arrow::Schema>> &HashJoinArrowKernel::getOutputSchema() const {
  return outputSchema_;
}

const std::optional<std::shared_ptr<TupleSet>> &HashJoinArrowKernel::getOutputBuffer() const {
  return outputBuffer_;
}

void HashJoinArrowKernel::clearOutputBuffer() {
  outputBuffer_.reset();
}

tl::expected<void, std::string> HashJoinArrowKernel::makeOutputSchema() {
  // check
  if (outputSchema_.has_value()) {
    return {};
  }
  if (!buildInputSchema_.has_value() || !probeInputSchema_.has_value()) {
    return {};
  }

  // join keys
  std::vector<arrow::FieldRef> buildJoinKeys, probeJoinKeys;
  for (const auto &leftColumn: pred_.getLeftColumnNames()) {
    buildJoinKeys.emplace_back(leftColumn);
  }
  for (const auto &rightColumn: pred_.getRightColumnNames()) {
    probeJoinKeys.emplace_back(rightColumn);
  }

  // output fields
  std::vector<arrow::FieldRef> buildOutputKeys, probeOutputKeys;
  arrow::FieldVector outputFields;
  // need to add probe fields first, because arrow's impl puts probe side on the left
  for (const auto &field: (*probeInputSchema_)->fields()) {
    if (neededColumnNames_.find(field->name()) != neededColumnNames_.end()) {
      probeOutputKeys.emplace_back(field->name());
      outputFields.emplace_back(field);
    }
  }
  for (const auto &field: (*buildInputSchema_)->fields()) {
    if (neededColumnNames_.find(field->name()) != neededColumnNames_.end()) {
      buildOutputKeys.emplace_back(field->name());
      outputFields.emplace_back(field);
    }
  }

  // hash-join node options
  arrow::compute::JoinType arrowJoinType;
  switch (joinType_) {
    case JoinType::INNER: {
      arrowJoinType = arrow::compute::JoinType::INNER;
      break;
    }
    // arrow's impl builds hash table using the right input while we do on the left, so we need to reverse
    case JoinType::LEFT: {
      arrowJoinType = arrow::compute::JoinType::RIGHT_OUTER;
      break;
    }
    // arrow's impl builds hash table using the right input while we do on the left, so we need to reverse
    case JoinType::RIGHT: {
      arrowJoinType = arrow::compute::JoinType::LEFT_OUTER;
      break;
    }
    case JoinType::FULL: {
      arrowJoinType = arrow::compute::JoinType::FULL_OUTER;
      break;
    }
    // calcite only emits left-semi join, corresponding to arrow's right-semi
    case JoinType::SEMI: {
      arrowJoinType = arrow::compute::JoinType::RIGHT_SEMI;
      break;
    }
    default: {
      return tl::make_unexpected("Unknown join type");
    }
  }
  hashJoinNodeOptions_ = arrow::compute::HashJoinNodeOptions{
          arrowJoinType,
          probeJoinKeys, buildJoinKeys,
          probeOutputKeys, buildOutputKeys
  };

  // output schema
  outputSchema_ = arrow::schema(outputFields);

  return {};
}

tl::expected<void, std::string> HashJoinArrowKernel::makeArrowExecPlan() {
  // check
  if (arrowExecPlanSuite_.has_value()) {
    return {};
  }
  if (!outputSchema_.has_value()) {
    return {};
  }
  if (joinType_ == JoinType::INNER || joinType_ == JoinType::SEMI) {
    // for inner or semi join, do not make arrow exec plan until both sides have input
    if (!buildInputBuffer_.has_value() || !probeInputBuffer_.has_value()) {
      return {};
    }
  } else {
    // for other joins, do not make arrow exec plan until one side has input
    if (!buildInputBuffer_.has_value() && !probeInputBuffer_.has_value()) {
      return {};
    }
  }

  // initialize
  auto execContext = std::make_shared<arrow::compute::ExecContext>(arrow::default_memory_pool());

  // exec plan
  auto expExecPlan = arrow::compute::ExecPlan::Make(execContext.get());
  if (!expExecPlan.ok()) {
    return tl::make_unexpected(expExecPlan.status().message());
  }
  auto execPlan = *expExecPlan;

  // two dummy input nodes at the beginning
  auto expBuildInputNode = arrow::compute::DummyNode::Make(execPlan.get(), *buildInputSchema_);
  if (!expBuildInputNode.ok()) {
    return tl::make_unexpected(expBuildInputNode.status().message());
  }
  auto buildInputNode = *expBuildInputNode;

  auto expProbeInputNode = arrow::compute::DummyNode::Make(execPlan.get(), *probeInputSchema_);
  if (!expProbeInputNode.ok()) {
    return tl::make_unexpected(expProbeInputNode.status().message());
  }
  auto probeInputNode = *expProbeInputNode;

  // hash-join node
  auto expHashJoinNode = arrow::compute::MakeExecNode("hashjoin", execPlan.get(), {probeInputNode, buildInputNode},
                                                      *hashJoinNodeOptions_);
  if (!expHashJoinNode.ok()) {
    return tl::make_unexpected(expHashJoinNode.status().message());
  }
  auto hashJoinNode = *expHashJoinNode;

  // buffered sink node at the end
  auto expBufferedSinkNode = arrow::compute::BufferedSinkNode::Make(
          execPlan.get(), {hashJoinNode}, *outputSchema_, DefaultBufferSize,
          [this] (const std::shared_ptr<TupleSet> &tupleSet) {
            return this->bufferOutput(tupleSet);
          });
  auto bufferedSinkNode = *expBufferedSinkNode;

  // save variables, start aggregate node and sink node
  arrowExecPlanSuite_ = HashJoinArrowExecPlanSuite{execContext, execPlan, buildInputNode, probeInputNode,
                                                   hashJoinNode, bufferedSinkNode, 0, 0};
  auto status = hashJoinNode->StartProducing();
  if (!status.ok()) {
    return tl::make_unexpected(status.message());
  }
  status = bufferedSinkNode->StartProducing();
  if (!status.ok()) {
    return tl::make_unexpected(status.message());
  }

  // consume buffered input
  if (buildInputBuffer_.has_value()) {
    auto res = consumeInput(*buildInputBuffer_, true);
    if (!res.has_value()) {
      return res;
    }
    buildInputBuffer_.reset();
  }
  if (buildInputFinalized_) {
    doFinalizeInput(true);
  }

  if (probeInputBuffer_.has_value()) {
    auto res = consumeInput(*probeInputBuffer_, false);
    if (!res.has_value()) {
      return res;
    }
    probeInputBuffer_.reset();
  }
  if (probeInputFinalized_) {
    doFinalizeInput(false);
  }

  return {};
}

tl::expected<void, std::string>
HashJoinArrowKernel::consumeInput(const std::shared_ptr<TupleSet> &tupleSet, bool isBuildSide) {
  // get corresponding input node
  auto& inputNode = isBuildSide ? arrowExecPlanSuite_->buildInputNode_ : arrowExecPlanSuite_->probeInputNode_;

  // read tupleSet into batches
  auto reader = std::make_shared<arrow::TableBatchReader>(*tupleSet->table());
  reader->set_chunksize(DefaultChunkSize);
  auto recordBatchReadResult = reader->Next();
  if (!recordBatchReadResult.ok()) {
    return tl::make_unexpected(recordBatchReadResult.status().message());
  }
  auto recordBatch = *recordBatchReadResult;

  // process batches
  int numInputBatches = 0;
  while (recordBatch) {
    ++numInputBatches;
    arrow::compute::ExecBatch execBatch(*recordBatch);
    arrowExecPlanSuite_->hashJoinNode_->InputReceived(inputNode, execBatch);

    // next batch
    recordBatchReadResult = reader->Next();
    if (!recordBatchReadResult.ok())
      return tl::make_unexpected(recordBatchReadResult.status().message());
    recordBatch = *recordBatchReadResult;
  }

  // update num input batches
  if (isBuildSide) {
    arrowExecPlanSuite_->numBuildInputBatches_ += numInputBatches;
  } else {
    arrowExecPlanSuite_->numProbeInputBatches_ += numInputBatches;
  }

  return {};
}

tl::expected<void, std::string> HashJoinArrowKernel::doFinalizeInput(bool isBuildSide) {
  auto& inputNode = isBuildSide ? arrowExecPlanSuite_->buildInputNode_ : arrowExecPlanSuite_->probeInputNode_;
  auto& numInputBatches = isBuildSide ? arrowExecPlanSuite_->numBuildInputBatches_ : arrowExecPlanSuite_->numProbeInputBatches_;

  // arrow's impl cannot handle the case of no input (crash), we have to check and put an empty batch into it
  if (numInputBatches == 0) {
    auto& inputSchema = isBuildSide ? *buildInputSchema_ : *probeInputSchema_;
    auto expEmptyRecordBatch = util::Util::makeEmptyRecordBatch(inputSchema);
    if (!expEmptyRecordBatch.has_value()) {
      return tl::make_unexpected(expEmptyRecordBatch.error());
    }

    arrow::compute::ExecBatch execBatch(**expEmptyRecordBatch);
    arrowExecPlanSuite_->hashJoinNode_->InputReceived(inputNode, execBatch);
    ++numInputBatches;
  }

  arrowExecPlanSuite_->hashJoinNode_->InputFinished(inputNode, numInputBatches);
  return {};
}

tl::expected<void, std::string> HashJoinArrowKernel::bufferInput(const std::shared_ptr<TupleSet> &tupleSet,
                                                                 bool isBuildSide) {
  auto& buffer = isBuildSide ? buildInputBuffer_ : probeInputBuffer_;
  if (!buffer.has_value()) {
    buffer = tupleSet;
    return {};
  } else {
    return (*buffer)->append(tupleSet);
  }
}

tl::expected<void, std::string> HashJoinArrowKernel::bufferOutput(const std::shared_ptr<TupleSet> &tupleSet) {
  if (!outputBuffer_.has_value()) {
    outputBuffer_ = tupleSet;
    return {};
  } else {
    return (*outputBuffer_)->append(tupleSet);
  }
}

void HashJoinArrowKernel::clear() {
  arrowExecPlanSuite_.reset();
  buildInputBuffer_.reset();
  probeInputBuffer_.reset();
  outputBuffer_.reset();
}

}

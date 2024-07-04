//
// Created by matt on 29/4/20.
//

#include <fpdb/executor/physical/join/hashjoin/HashJoinProbePOp.h>
#include <fpdb/executor/physical/join/hashjoin/HashJoinProbeKernel.h>
#include <fpdb/executor/physical/join/hashjoin/HashSemiJoinProbeKernel.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/message/TupleSetIndexMessage.h>
#include <fpdb/tuple/arrow/SchemaHelper.h>
#include <utility>

using namespace fpdb::executor::physical::join;

HashJoinProbePOp::HashJoinProbePOp(string name,
                                   vector<string> projectColumnNames,
                                   int nodeId,
                                   const HashJoinPredicate& pred,
                                   JoinType joinType) :
	PhysicalOp(move(name), HASH_JOIN_PROBE, move(projectColumnNames), nodeId) {

  set<string> neededColumnNames(getProjectColumnNames().begin(), getProjectColumnNames().end());
  switch (joinType) {
    case INNER: {
      kernel_ = HashJoinProbeKernel::make(pred, move(neededColumnNames), false, false);
      break;
    }
    case LEFT: {
      kernel_ = HashJoinProbeKernel::make(pred, move(neededColumnNames), true, false);
      break;
    }
    case RIGHT: {
      kernel_ = HashJoinProbeKernel::make(pred, move(neededColumnNames), false, true);
      break;
    }
    case FULL: {
      kernel_ = HashJoinProbeKernel::make(pred, move(neededColumnNames), true, true);
      break;
    }
    case SEMI: {
      kernel_ = HashSemiJoinProbeKernel::make(pred, move(neededColumnNames));
      break;
    }
    default:
      // This is not inside actor processing, so we cannot use ctx()->notifyError()
      throw runtime_error(fmt::format("Unsupported hash join type, {}", joinType));
  }
}

std::string HashJoinProbePOp::getTypeString() const {
  return "HashJoinProbePOp";
}

void HashJoinProbePOp::onReceive(const Envelope &msg) {
  if (msg.message().type() == MessageType::START) {
	  this->onStart();
  } else if (msg.message().type() == MessageType::TUPLESET) {
    auto tupleSetMessage = dynamic_cast<const TupleSetMessage &>(msg.message());
    this->onTupleSet(tupleSetMessage);
  } else if (msg.message().type() == MessageType::TUPLESET_INDEX) {
    auto hashTableMessage = dynamic_cast<const TupleSetIndexMessage &>(msg.message());
    this->onHashTable(hashTableMessage);
  } else if (msg.message().type() == MessageType::COMPLETE) {
    auto completeMessage = dynamic_cast<const CompleteMessage &>(msg.message());
    this->onComplete(completeMessage);
  } else {
	  ctx()->notifyError(fmt::format("Unrecognized message type: {}, {}", msg.message().getTypeString(), name()));
  }
}

void HashJoinProbePOp::onStart() {
  SPDLOG_DEBUG("Starting operator  |  name: '{}'", this->name());
}

void HashJoinProbePOp::onTupleSet(const TupleSetMessage &msg) {
  // Incremental join immediately
  const auto& tupleSet = msg.tuples();
  auto result = kernel_->joinProbeTupleSet(tupleSet);
  if(!result)
    ctx()->notifyError(fmt::format("{}, {}", result.error(), name()));

  // Send
  send(false);
}

void HashJoinProbePOp::onComplete(const CompleteMessage &) {
  if (!ctx()->isComplete() && ctx()->operatorMap().allComplete(POpRelationshipType::Producer)) {
    // Finalize
    auto result = kernel_->finalize();
    if (!result) {
      ctx()->notifyError(result.error());
    }

    // Send final tupleSet
    send(true);

    // Send empty if no result
    if (!sentResult) {
      sendEmpty();
    }

    // Complete
  	ctx()->notifyComplete();
  }
}

void HashJoinProbePOp::onHashTable(const TupleSetIndexMessage &msg) {
  // Incremental join immediately
  auto result = kernel_->joinBuildTupleSetIndex(msg.getTupleSetIndex());
  if(!result)
    ctx()->notifyError(fmt::format("{}, {}", result.error(), name()));

  // Send
  send(false);
}

void HashJoinProbePOp::send(bool force) {
  auto buffer = kernel_->getBuffer();
  if (buffer.has_value()) {
    auto numRows = buffer.value()->numRows();
    if (numRows >= DefaultBufferSize || (force && numRows > 0)) {
      // Here no need to project buffer using projectColumnNames as it won't have redundant columns
      auto tupleSet = TupleSet::make(buffer.value()->table());
      shared_ptr<Message> tupleSetMessage = make_shared<TupleSetMessage>(tupleSet, name());
      ctx()->tell(tupleSetMessage);
      sentResult = true;
      kernel_->clearBuffer();
    }
  }
}

void HashJoinProbePOp::sendEmpty() {
  auto outputSchema = kernel_->getOutputSchema();
  if (!outputSchema.has_value()) {
    ctx()->notifyError("OutputSchema not set yet");
  }
  auto tupleSet = TupleSet::make(outputSchema.value());
  shared_ptr<Message> tupleSetMessage = make_shared<TupleSetMessage>(tupleSet, name());
  ctx()->tell(tupleSetMessage);
}

void HashJoinProbePOp::clear() {
  kernel_->clear();
}

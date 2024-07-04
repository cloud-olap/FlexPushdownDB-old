//
// Created by Yifei Yang on 4/27/22.
//

#include <fpdb/executor/physical/join/hashjoin/HashJoinArrowPOp.h>

namespace fpdb::executor::physical::join {

HashJoinArrowPOp::HashJoinArrowPOp(const std::string &name,
                                   const std::vector<std::string> &projectColumnNames,
                                   int nodeId,
                                   const HashJoinPredicate &pred,
                                   JoinType joinType):
  PhysicalOp(name, HASH_JOIN_ARROW, projectColumnNames, nodeId),
  kernel_(HashJoinArrowKernel::make(pred, {projectColumnNames.begin(), projectColumnNames.end()}, joinType)) {}

std::string HashJoinArrowPOp::getTypeString() const {
  return "HashJoinArrowPOp";
}

void HashJoinArrowPOp::onReceive(const Envelope &msg) {
  if (msg.message().type() == MessageType::START) {
    this->onStart();
  } else if (msg.message().type() == MessageType::TUPLESET) {
    auto tupleMessage = dynamic_cast<const TupleSetMessage &>(msg.message());
    this->onTupleSet(tupleMessage);
  } else if (msg.message().type() == MessageType::COMPLETE) {
    auto completeMessage = dynamic_cast<const CompleteMessage &>(msg.message());
    this->onComplete(completeMessage);
  } else {
    ctx()->notifyError("Unrecognized message type " + msg.message().getTypeString());
  }
}

void HashJoinArrowPOp::clearProducers() {
  PhysicalOp::clearProducers();
  buildProducers_.clear();
  probeProducers_.clear();
}

const HashJoinArrowKernel &HashJoinArrowPOp::getKernel() const {
  return kernel_;
}

const std::set<std::string> &HashJoinArrowPOp::getBuildProducers() const {
  return buildProducers_;
}

const std::set<std::string> &HashJoinArrowPOp::getProbeProducers() const {
  return probeProducers_;
}

void HashJoinArrowPOp::setBuildProducers(const std::set<std::string> &buildProducers) {
  buildProducers_ = buildProducers;
}

void HashJoinArrowPOp::setProbeProducers(const std::set<std::string> &probeProducers) {
  probeProducers_ = probeProducers;
}

void HashJoinArrowPOp::addBuildProducer(const std::shared_ptr<PhysicalOp> &buildProducer) {
  buildProducers_.emplace(buildProducer->name());
  consume(buildProducer);
}

void HashJoinArrowPOp::addProbeProducer(const std::shared_ptr<PhysicalOp> &probeProducer) {
  probeProducers_.emplace(probeProducer->name());
  consume(probeProducer);
}

void HashJoinArrowPOp::clearBuildProducers() {
  buildProducers_.clear();
  producers_ = probeProducers_;
}

void HashJoinArrowPOp::clearProbeProducers() {
  probeProducers_.clear();
  producers_ = buildProducers_;
}

void HashJoinArrowPOp::onStart() {
  SPDLOG_DEBUG("Starting operator  |  name: '{}'", this->name());
}

void HashJoinArrowPOp::onComplete(const CompleteMessage &message) {
  // check build/probe side
  auto sender = message.sender();
  if (buildProducers_.find(sender) != buildProducers_.end()) {
    if (++numCompletedBuildProducers_ >= (int) buildProducers_.size()) {
      kernel_.finalizeInput(true);
    }
  } else if (probeProducers_.find(sender) != probeProducers_.end()) {
    if (++numCompletedProbeProducers_ >= (int) probeProducers_.size()) {
      kernel_.finalizeInput(false);
    }
  } else {
    ctx()->notifyError(fmt::format("Unknown sender '{}', neither build nor probe producer", sender));
  }

  // check all complete
  if (!ctx()->isComplete() && ctx()->operatorMap().allComplete(POpRelationshipType::Producer)) {
    // send final output
    send();

    // check if never sent result
    if (!sentResult) {
      sendEmpty();
    }

    // complete
    ctx()->notifyComplete();
  }
}

void HashJoinArrowPOp::onTupleSet(const TupleSetMessage &message) {
  auto tupleSet = message.tuples();
  auto sender = message.sender();

  // put input into kernel according to build/probe side
  tl::expected<void, string> result;
  if (buildProducers_.find(sender) != buildProducers_.end()) {
    result = kernel_.joinBuildTupleSet(tupleSet);
  } else if (probeProducers_.find(sender) != probeProducers_.end()) {
    result = kernel_.joinProbeTupleSet(tupleSet);
  } else {
    ctx()->notifyError(fmt::format("Unknown sender '{}', neither build nor probe producer", sender));
  }
  if (!result.has_value()) {
    ctx()->notifyError(result.error());
  }

  // send output
  send();
}

void HashJoinArrowPOp::send() {
  auto outputBuffer = kernel_.getOutputBuffer();

  if (outputBuffer.has_value()) {
    // it's guaranteed that buffer has exceed DefaultBufferSize inside kernel, so no need to check here
    // already been projected inside kernel, so no need to do it here
    std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(*outputBuffer, name_);
    ctx()->tell(tupleSetMessage);
    sentResult = true;
    kernel_.clearOutputBuffer();
  }
}

void HashJoinArrowPOp::sendEmpty() {
  auto outputSchema = kernel_.getOutputSchema();
  if (!outputSchema.has_value()) {
    ctx()->notifyError("Output schema not set yet on completing");
  }
  std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(TupleSet::make(*outputSchema), name_);
  ctx()->tell(tupleSetMessage);
  sentResult = true;
}

void HashJoinArrowPOp::clear() {
  kernel_.clear();
}

}

//
// Created by matt on 5/12/19.
//

#include <fpdb/executor/physical/POpContext.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterUseKernel.h>
#include <fpdb/executor/cache/SegmentCacheActor.h>
#include <fpdb/executor/message/Message.h>
#include <fpdb/executor/message/CompleteMessage.h>
#include <fpdb/executor/message/TupleSetMessage.h>
#include <fpdb/executor/message/TupleSetSizeMessage.h>
#include <fpdb/executor/message/cache/LoadRequestMessage.h>
#include <fpdb/executor/message/cache/CacheMetricsMessage.h>
#include <fpdb/executor/flight/FlightHandler.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <cassert>

using namespace fpdb::executor::message;
using namespace fpdb::executor::flight;

namespace fpdb::executor::physical {

void POpContext::tell(const std::shared_ptr<Message> &msg,
                      const std::optional<std::set<std::string>> &consumers) {
  assert(this);

  if (complete_) {
    notifyError(fmt::format("Cannot tell message to consumers, operator {} ('{}') is complete",
                            this->operatorActor()->id(),
                            this->operatorActor()->operator_()->name()));
  }

  // Send message to consumers
  auto &finalConsumers = consumers.has_value() ? *consumers : this->operatorActor()->operator_()->consumers();
  for (const auto &consumer: finalConsumers) {
    send_regular(msg, consumer);
  }
}

void POpContext::send(const std::shared_ptr<message::Message> &msg, const std::string& consumer) {
  message::Envelope e(msg);

  if (consumer == "SegmentCache"){
    // message to segment cache
    if (msg->type() == MessageType::LOAD_REQUEST) {
      operatorActor_->request(segmentCacheActor_, infinite, LoadAtom_v, std::static_pointer_cast<fpdb::executor::message::LoadRequestMessage>(msg))
      .then([=](const std::shared_ptr<fpdb::executor::message::LoadResponseMessage>& response){
      operatorActor_->anon_send(this->operatorActor(), Envelope(response));
      });
    }
    else if (msg->type() == MessageType::STORE_REQUEST) {
      operatorActor_->anon_send(segmentCacheActor_, StoreAtom_v, std::static_pointer_cast<fpdb::executor::message::StoreRequestMessage>(msg));
    }
    else if (msg->type() == MessageType::WEIGHT_REQUEST) {
      operatorActor_->anon_send(segmentCacheActor_, WeightAtom_v, std::static_pointer_cast<fpdb::executor::message::WeightRequestMessage>(msg));
    }
    else if (msg->type() == MessageType::CACHE_METRICS) {
      operatorActor_->anon_send(segmentCacheActor_, MetricsAtom_v, std::static_pointer_cast<fpdb::executor::message::CacheMetricsMessage>(msg));
    }
    else {
      notifyError("Unrecognized message " + msg->getTypeString());
    }
  } else {
    // regular message
    send_regular(msg, consumer);
  }
}

void POpContext::send_regular(const std::shared_ptr<message::Message> &msg, const std::string &consumer) {
  // get local op entry
  auto expOpEntry = operatorMap_.get(consumer);
  if (!expOpEntry.has_value()) {
    notifyError(expOpEntry.error());
    return;
  }
  auto opEntry = *expOpEntry;

  // apply embedded bloom filter
  auto appliedMsg = applyEmbeddedBloomFilter(msg, consumer);

  // decide whether use actor's comm or flight to send tupleSet message
  if (USE_FLIGHT_COMM
      && appliedMsg->type() == MessageType::TUPLESET
      && operatorActor_->operator_()->getNodeId() != opEntry.getNodeId()) {
    // check if daemon flight server already created
    if (FlightHandler::daemonServer_ == nullptr) {
      notifyError("Daemon flight server not created yet");
    }

    // put table into flight server
    FlightHandler::daemonServer_->putTable(operatorActor_->operator_()->getQueryId(), operatorActor_->name_, consumer,
                                           std::static_pointer_cast<TupleSetMessage>(appliedMsg)->tuples()->table());

    // send notification
    std::shared_ptr<Message> tupleSetReadyRemoteMessage = std::make_shared<TupleSetReadyRemoteMessage>(
            FlightHandler::daemonServer_->getHost(),
            FlightHandler::daemonServer_->getPort(),
            false,
            operatorActor_->name_);
    operatorActor_->anon_send(opEntry.getActor(), Envelope(tupleSetReadyRemoteMessage));
  } else {
    // send using actor's comm
    operatorActor_->anon_send(opEntry.getActor(), Envelope(appliedMsg));

    // metrics
#if SHOW_DEBUG_METRICS == true
    if (appliedMsg->type() == MessageType::TUPLESET
        && operatorActor_->operator_()->getNodeId() != opEntry.getNodeId()) {
      std::shared_ptr<Message> execMetricsMsg = std::make_shared<TransferMetricsMessage>(
              metrics::TransferMetrics(0, 0, std::static_pointer_cast<TupleSetMessage>(appliedMsg)->tuples()->size()),
              operatorActor_->name_);
      operatorActor_->anon_send(rootActor_, Envelope(execMetricsMsg));
    }
#endif
  }
}

/**
 * Send a CompleteMessage to all consumers and the root actor
 */
void POpContext::notifyComplete() {
  SPDLOG_DEBUG("Completing operator  |  source: {} ('{}')", this->operatorActor()->id(), this->operatorActor()->operator_()->name());
  if(complete_)
    notifyError(fmt::format("Cannot complete already completed operator {} ('{}')", this->operatorActor()->id(), this->operatorActor()->operator_()->name()));

  POpActor* operatorActor = this->operatorActor();

  std::shared_ptr<message::Message> msg = std::make_shared<message::CompleteMessage>(operatorActor->operator_()->name());
  message::Envelope e(msg);

  // Send message to consumers
  for(const auto& consumer: this->operatorActor()->operator_()->consumers()){
    ::caf::actor actorHandle = operatorMap_.get(consumer).value().getActor();
    operatorActor->anon_send(actorHandle, e);
  }

  // Send message to root actor
  operatorActor->anon_send(rootActor_, e);

  // Clear internal state, except collate whose internal state is final result
  if (operatorActor->operator_()->getType() != POpType::COLLATE) {
    operatorActor->operator_()->clear();
  }

  complete_ = true;
  operatorActor->running_ = false;
}

/**
 * Send error to the root actor
 */
void POpContext::notifyError(const std::string &content) {
  std::shared_ptr<Message> errorMsg = std::make_shared<ErrorMessage>(content, operatorActor_->name());
  message::Envelope e(errorMsg);
  operatorActor_->anon_send(rootActor_, e);
  operatorActor_->on_exit();
}

/**
 * Send msg to the root actor
 * @param msg
 */
void POpContext::notifyRoot(const std::shared_ptr<message::Message> &msg,
                            const std::optional<std::string> &embeddedBloomFilterConsumer) {
  // apply embedded bloom filter if needed
  auto finalMsg = embeddedBloomFilterConsumer.has_value() ?
                  applyEmbeddedBloomFilter(msg, *embeddedBloomFilterConsumer) : msg;
  message::Envelope e(finalMsg);
  operatorActor_->anon_send(rootActor_, e);
}

POpContext::POpContext(::caf::actor rootActor, ::caf::actor segmentCacheActor):
    operatorActor_(nullptr),
    rootActor_(std::move(rootActor)),
    segmentCacheActor_(std::move(segmentCacheActor))
{}

LocalPOpDirectory &POpContext::operatorMap() {
  return operatorMap_;
}

POpActor* POpContext::operatorActor() {
  return operatorActor_;
}

void POpContext::operatorActor(POpActor *operatorActor) {
  operatorActor_ = operatorActor;
}

bool POpContext::isComplete() const {
  return complete_;
}

void POpContext::destroyActorHandles() {
  operatorMap_.destroyActorHandles();
  destroy(rootActor_);
  destroy(segmentCacheActor_);
}

std::shared_ptr<message::Message> POpContext::applyEmbeddedBloomFilter(const std::shared_ptr<message::Message> &msg,
                                                                       const std::string &consumer) {
  // check if applicable
  if (msg->type() != MessageType::TUPLESET && msg->type() != MessageType::TUPLESET_BUFFER) {
    return msg;
  }

  auto consumerToBloomFilterInfo = operatorActor_->operator_()->getConsumerToBloomFilterInfo();
  if (consumerToBloomFilterInfo.empty()) {
    return msg;
  }
  auto bloomFilterInfoIt = consumerToBloomFilterInfo.find(consumer);
  if (bloomFilterInfoIt == consumerToBloomFilterInfo.end()) {
    return msg;
  }
  const auto &bloomFilterInfo = bloomFilterInfoIt->second;
  if (!bloomFilterInfo->bloomFilter_.has_value()) {
    return msg;
  }

  // apply bloom filter to the tupleSet
  std::shared_ptr<TupleSet> tupleSet;
  if (msg->type() == MessageType::TUPLESET) {
    tupleSet = std::static_pointer_cast<TupleSetMessage>(msg)->tuples();
  } else {
    tupleSet = std::static_pointer_cast<TupleSetBufferMessage>(msg)->tuples();
  }
  auto expFilteredTupleSet = bloomfilter::BloomFilterUseKernel::filter(tupleSet,
                                                                       *bloomFilterInfo->bloomFilter_,
                                                                       bloomFilterInfo->columnNames_);
  if (!expFilteredTupleSet.has_value()) {
    notifyError((expFilteredTupleSet.error()));
  }

  // return message with filtered tupleSet
  if (msg->type() == MessageType::TUPLESET) {
    return std::make_shared<TupleSetMessage>(*expFilteredTupleSet, msg->sender());
  } else {
    return std::make_shared<TupleSetBufferMessage>(*expFilteredTupleSet,
                                                   std::static_pointer_cast<TupleSetBufferMessage>(msg)->getConsumer(),
                                                   msg->sender());
  }
}

} // namespace

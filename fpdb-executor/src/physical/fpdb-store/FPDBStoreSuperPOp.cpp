//
// Created by Yifei Yang on 2/21/22.
//

#include <fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOp.h>
#include <fpdb/executor/physical/filter/FilterPOp.h>
#include <fpdb/executor/physical/serialization/PhysicalPlanSerializer.h>
#include <fpdb/executor/physical/serialization/PhysicalPlanDeserializer.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/flight/FlightClients.h>
#include <fpdb/executor/flight/FlightHandler.h>
#include <fpdb/executor/message/TransferMetricsMessage.h>
#include <fpdb/executor/message/PushdownFallBackMessage.h>
#include <fpdb/executor/metrics/Globals.h>
#include <fpdb/executor/caf/CAFAdaptPushdownUtil.h>
#include <fpdb/executor/FPDBStoreExecution.h>
#include <fpdb/store/server/flight/SelectObjectContentTicket.hpp>
#include <fpdb/store/server/flight/GetTableTicket.hpp>
#include <fpdb/store/server/flight/ClearBitmapCmd.hpp>
#include <fpdb/store/server/flight/ClearTableCmd.hpp>
#include <fpdb/store/server/flight/adaptive/PushbackCompleteCmd.hpp>
#include <fpdb/store/server/flight/Util.hpp>
#include <fpdb/util/Util.h>
#include <arrow/flight/api.h>
#include <unordered_map>

using namespace fpdb::store::server::flight;

namespace fpdb::executor::physical::fpdb_store {

FPDBStoreSuperPOp::FPDBStoreSuperPOp(const std::string &name,
                                     const std::vector<std::string> &projectColumnNames,
                                     int nodeId,
                                     const std::shared_ptr<PhysicalPlan> &subPlan,
                                     int parallelDegree,
                                     const std::string &host,
                                     int fileServicePort,
                                     int flightPort):
  PhysicalOp(name, POpType::FPDB_STORE_SUPER, projectColumnNames, nodeId),
  subPlan_(subPlan),
  parallelDegree_(parallelDegree),
  host_(host),
  fileServicePort_(fileServicePort),
  flightPort_(flightPort) {}

FPDBStoreSuperPOp::~FPDBStoreSuperPOp() {
  // wait pushback double-exec if enabled before destruction
  if (EnablePushbackTailReqDoubleExec) {
    OpsWithPushbackExec.erase(makePushbackDoubleExecKey(queryId_, name_));
    if (doubleExecCv_ != nullptr) {
      std::unique_lock lock(*doubleExecMutex_);
      doubleExecCv_->wait(lock, [&] {
        return isDoubleExecFinished_;
      });
    }
  }
}

void FPDBStoreSuperPOp::onReceive(const Envelope &envelope) {
  const auto &message = envelope.message();

  if (message.type() == MessageType::START) {
    this->onStart();
  } else if (message.type() == MessageType::SCAN) {
    auto scanMessage = dynamic_cast<const ScanMessage &>(message);
    this->onCacheLoadResponse(scanMessage);
  } else if (message.type() == MessageType::BLOOM_FILTER) {
    auto bloomFilterMessage = dynamic_cast<const BloomFilterMessage &>(message);
    this->onBloomFilter(bloomFilterMessage);
  } else if (message.type() == MessageType::COMPLETE) {
    // noop
  } else {
    ctx()->notifyError("Unrecognized message type " + message.getTypeString());
  }
}

std::string FPDBStoreSuperPOp::getTypeString() const {
  return "FPDBStoreSuperPOp";
}

void FPDBStoreSuperPOp::produce(const std::shared_ptr<PhysicalOp> &op) {
  PhysicalOp::produce(op);

  // need to add op to consumerVec_ of shuffle op explicitly
  if (shufflePOpName_.has_value()) {
    auto expShufflePOp = subPlan_->getPhysicalOp(*shufflePOpName_);
    if (!expShufflePOp.has_value()) {
      throw std::runtime_error(expShufflePOp.error());
    }
    std::static_pointer_cast<shuffle::ShufflePOp>(*expShufflePOp)->addToConsumerVec(op);
  }
}

const std::shared_ptr<PhysicalPlan> &FPDBStoreSuperPOp::getSubPlan() const {
  return subPlan_;
}

const std::string &FPDBStoreSuperPOp::getHost() const {
  return host_;
}

int FPDBStoreSuperPOp::getFileServicePort() const {
  return fileServicePort_;
}

int FPDBStoreSuperPOp::getFlightPort() const {
  return flightPort_;
}

void FPDBStoreSuperPOp::setWaitForScanMessage(bool waitForScanMessage) {
  waitForScanMessage_ = waitForScanMessage;
}

void FPDBStoreSuperPOp::setReceiveByOthers(bool receiveByOthers) {
  receiveByOthers_ = receiveByOthers;
}

void FPDBStoreSuperPOp::setShufflePOp(const std::shared_ptr<PhysicalOp> &op) {
  shufflePOpName_ = op->name();
}

void FPDBStoreSuperPOp::addFPDBStoreBloomFilterProducer(
        const std::shared_ptr<PhysicalOp> &fpdbStoreBloomFilterProducer) {
  ++numBloomFiltersExpected_;
  PhysicalOp::consume(fpdbStoreBloomFilterProducer);
}

void FPDBStoreSuperPOp::setForwardConsumers(const std::vector<std::shared_ptr<PhysicalOp>> &consumers) {
  auto expRootPOp = subPlan_->getRootPOp();
  if (!expRootPOp.has_value()) {
    throw std::runtime_error(expRootPOp.error());
  }
  auto collatePOp = std::static_pointer_cast<collate::CollatePOp>(*expRootPOp);
  const auto &producers = collatePOp->producers();
  if (producers.size() != consumers.size()) {
    throw std::runtime_error(fmt::format("num producers ({}) and num forward consumers ({}) mismatch on \"set()\"",
                             producers.size(), consumers.size()));
  }
  // set both forwardConsumers and endConsumers for root
  std::vector<std::string> endConsumers;
  std::unordered_map<std::string, std::string> forwardConsumerMap;
  int i = 0;
  for (const auto &producer: producers) {
    const auto &consumer = consumers[i++]->name();
    forwardConsumerMap[producer] = consumer;
    endConsumers.emplace_back(consumer);
  }
  collatePOp->setForward(true);
  collatePOp->setForwardConsumers(forwardConsumerMap);
  collatePOp->setEndConsumers(endConsumers);
}

void FPDBStoreSuperPOp::resetForwardConsumers() {
  auto expRootPOp = subPlan_->getRootPOp();
  if (!expRootPOp.has_value()) {
    throw std::runtime_error(expRootPOp.error());
  }
  auto collatePOp = std::static_pointer_cast<collate::CollatePOp>(*expRootPOp);
  const auto &producers = collatePOp->producers();
  const auto &endConsumers = collatePOp->getEndConsumers();
  if (producers.size() != endConsumers.size()) {
    throw std::runtime_error(fmt::format("num producers ({}) and num forward consumers ({}) mismatch on \"reset()\"",
                                         producers.size(), endConsumers.size()));
  }
  // reset forwardConsumers
  std::unordered_map<std::string, std::string> forwardConsumerMap;
  int i = 0;
  for (const auto &producer: producers) {
    forwardConsumerMap[producer] = endConsumers[i++];
  }
  collatePOp->setForwardConsumers(forwardConsumerMap);
}

void FPDBStoreSuperPOp::setGetAdaptPushdownMetrics(bool getAdaptPushdownMetrics) {
  getAdaptPushdownMetrics_ = getAdaptPushdownMetrics;
}

void FPDBStoreSuperPOp::unBlockNonRestrictFilters(const std::string &filterPOpName,
                                                  const std::string &fpdbStoreSuperPOpName) {
  std::unique_lock lock(FPDBStoreSuperPOpDetachMutex);

  nonRestrictFilters.emplace(filterPOpName);
  const auto cvIt = FPDBStoreSuperPOpDetachCvs.find(fpdbStoreSuperPOpName);
  if (cvIt != FPDBStoreSuperPOpDetachCvs.end()) {
    cvIt->second->notify_one();
  }
}

void FPDBStoreSuperPOp::processDetachIn() {
  std::unique_lock lock(FPDBStoreSuperPOpDetachMutex);

  // set filterPOpNames_
  for (const auto &opIt: subPlan_->getPhysicalOps()) {
    if (opIt.second->getType() == POpType::FILTER) {
      filterPOpNames_.emplace(opIt.first);
    }
  }

  // wait until satisfied
  auto cv = std::make_shared<std::condition_variable_any>();
  FPDBStoreSuperPOpDetachCvs[name_] = cv;
  cv->wait(lock, [&] {
    // check if its filters are all in nonRestrictFilters
    if (fpdb::util::isSubSet(filterPOpNames_, nonRestrictFilters)) {
      for (const auto &opName: filterPOpNames_) {
        nonRestrictFilters.erase(opName);
      }
      return true;
    }
    // if false, check number of available slots
    return numFPDBStoreSuperPOpDetachSlots > 0;
  });
  FPDBStoreSuperPOpDetachCvs.erase(name_);

  --numFPDBStoreSuperPOpDetachSlots;
}

void FPDBStoreSuperPOp::processDetachOut() {
  std::unique_lock lock(FPDBStoreSuperPOpDetachMutex);
  ++numFPDBStoreSuperPOpDetachSlots;

  // randomly weak up one
  if (!FPDBStoreSuperPOpDetachCvs.empty()) {
    FPDBStoreSuperPOpDetachCvs.begin()->second->notify_one();
  }
}

void FPDBStoreSuperPOp::onStart() {
  SPDLOG_DEBUG("Starting operator  |  name: '{}'", this->name());

  // try to process
  if (readyToProcess()) {
    processAtStore();
  }
}

void FPDBStoreSuperPOp::onCacheLoadResponse(const ScanMessage &msg) {
  // check if nothing to process (i.e. no columns to scan)
  // note: it's possible that scan columns is not empty but project columns is, when the storage side purely
  // constructs filter bitmap without returning any data
  auto scanColumnNames = msg.getScanColumnNames();
  if (scanColumnNames.empty()) {
    processEmpty();
    return;
  }
  auto projectColumnNames = msg.getProjectColumnNames();

  // set scan column names
  for (const auto &opIt: subPlan_->getPhysicalOps()) {
    auto op = opIt.second;
    if (op->getType() == POpType::FPDB_STORE_FILE_SCAN) {
      op->setProjectColumnNames(scanColumnNames);
      break;
    }
  }

  // set project column names
  auto expRootOp = subPlan_->getRootPOp();
  if (!expRootOp.has_value()) {
    ctx()->notifyError(expRootOp.error());
    return;
  }
  (*expRootOp)->setProjectColumnNames(projectColumnNames);

  // set flag
  waitForScanMessage_ = false;

  // try to process
  if (readyToProcess()) {
    processAtStore();
  }
}

void FPDBStoreSuperPOp::onBloomFilter(const BloomFilterMessage &) {
  // this is just to notify that one bloom filter has been sent to store, no real bloom filter in the msg
  ++numBloomFiltersReceived_;

  // try to process
  if (readyToProcess()) {
    processAtStore();
  }
}

bool FPDBStoreSuperPOp::readyToProcess() {
  // check if waiting for scan message, specifically in hybrid mode
  if (waitForScanMessage_) {
    return false;
  }

  // check if all bloom filters needed have been sent to store
  if (numBloomFiltersReceived_ < numBloomFiltersExpected_) {
    return false;
  }

  return true;
}

void FPDBStoreSuperPOp::processAtStore(bool isDoubleExec) {
  // create double-exec mutex if not yet
  if (EnablePushbackTailReqDoubleExec) {
    if (doubleExecMutex_ == nullptr) {
      doubleExecMutex_ = std::make_shared<std::mutex>();
    }
  }

  // for limiting number of concurrent detached FPDBStoreSuperPOp
  if (ENABLE_FILTER_BITMAP_PUSHDOWN) {
    processDetachIn();
  }

  // make flight client and connect
  auto client = flight::GlobalFlightClients.getFlightClient(host_, flightPort_);

  // send request to store
  auto expPlanString = serialize(false);
  if (!expPlanString.has_value()) {
    onErrorDuringProcess(expPlanString.error());
    return;
  }

  const auto &flightDaemonServer = executor::flight::FlightHandler::daemonServer_;
  auto ticketObj = SelectObjectContentTicket::make(queryId_, name_, *expPlanString, parallelDegree_,
          flightDaemonServer != nullptr ?
              ReqExtraInfo(isDoubleExec, flightDaemonServer->getHost(), flightDaemonServer->getPort()):
              ReqExtraInfo());
  auto expTicket = ticketObj->to_ticket(false);
  if (!expTicket.has_value()) {
    onErrorDuringProcess(expTicket.error());
    return;
  }

  // if pushdown result should be received by consumers in pipeline, let them start waiting now
  // currently only true when hashjoin pushdown is enabled
  if (receiveByOthers_) {
    std::shared_ptr<Message> tupleSetWaitRemoteMessage =
            std::make_shared<TupleSetWaitRemoteMessage>(host_, flightPort_, name_);
    ctx()->tell(tupleSetWaitRemoteMessage);
  }

  auto startTime = std::chrono::steady_clock::now();
  std::unique_ptr<::arrow::flight::FlightStreamReader> reader;
  auto status = client->DoGet(*expTicket, &reader);
  if (!status.ok()) {
    auto flightStatusDetail = std::static_pointer_cast<arrow::flight::FlightStatusDetail>(status.detail());
    // the request is rejected by storage due to resource limitation
    if (ENABLE_ADAPTIVE_PUSHDOWN && flightStatusDetail->code() == ReqRejectStatusCode) {
      // currently unsupported if "receiveByOthers_" = true
      if (receiveByOthers_) {
        onErrorDuringProcess("Adaptive pushdown with pipelining pushdown results is currently unsupported");
        return;
      }
      // fall back to pullup (adaptive pushdown)
      bool isResultNeeded;
      processAsPullup(&isResultNeeded);
      // if using adapt pushdown manager v2 or v3, signal the storage that pushback has completed
      if (AdaptPushdownManagerV == AdaptPushdownManagerVersion::V2 ||
          AdaptPushdownManagerV == AdaptPushdownManagerVersion::V3) {
        // send request to store
        auto cmdObj = PushbackCompleteCmd::make(queryId_, name_);
        auto expCmd = cmdObj->serialize(false);
        if (!expCmd.has_value()) {
          ctx()->notifyError(expCmd.error());
          return;
        }
        auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
        std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
        std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
        status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
        if (!status.ok()) {
          ctx()->notifyError(status.message());
          return;
        }
        status = writer->Close();
        if (!status.ok()) {
          ctx()->notifyError(status.message());
          return;
        }
      }

      // complete and return
      if (isResultNeeded) {
        ctx()->notifyComplete();
      }
      if (ENABLE_FILTER_BITMAP_PUSHDOWN) {
        processDetachOut();
      }
      return;
    }

    // error
    else {
      onErrorDuringProcess(status.message());
      return;
    }
  }

  std::shared_ptr<::arrow::Table> table;
  if (!receiveByOthers_ && !shufflePOpName_.has_value()) {
    status = reader->ReadAll(&table);
    if (!status.ok()) {
      onErrorDuringProcess(status.message());
      return;
    }
  }
  auto stopTime = std::chrono::steady_clock::now();

  // for metrics of adaptive pushdown
  if (getAdaptPushdownMetrics_) {
    auto expAdaptPushdownMetricsKey = AdaptPushdownMetricsMessage::generateAdaptPushdownMetricsKey(queryId_, name_);
    if (!expAdaptPushdownMetricsKey.has_value()) {
      onErrorDuringProcess(expAdaptPushdownMetricsKey.error());
      return;
    }
    int64_t execTime = std::chrono::duration_cast<chrono::nanoseconds>(stopTime - startTime).count();
    std::shared_ptr<Message> adaptPushdownMetricsMessage = std::make_shared<AdaptPushdownMetricsMessage>(
            *expAdaptPushdownMetricsKey, execTime, name_);
    ctx()->notifyRoot(adaptPushdownMetricsMessage);
  }

  // ignore the slower output if enabling double-exec
  bool isResultNeeded = true;
  if (EnablePushbackTailReqDoubleExec) {
    std::unique_lock lock(*doubleExecMutex_);
    if (isOneExecFinished_) {
      isResultNeeded = false;
    }
    isOneExecFinished_ = true;
  }

  // if pushdown result hasn't been waiting by consumers
  if (isResultNeeded) {
    if (!receiveByOthers_) {
      // if not having shuffle op, do regularly
      if (!shufflePOpName_.has_value()) {
        // check table
        if (table == nullptr) {
          onErrorDuringProcess("Received null table from FPDB-Store");
          return;
        }

        // send output tupleSet
        std::shared_ptr<TupleSet> tupleSet;
        if (table->num_rows() > 0) {
          tupleSet = TupleSet::make(table);
        } else {
          tupleSet = TupleSet::make(table->schema());
        }
        std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(tupleSet, name_);
        ctx()->tell(tupleSetMessage);

        // metrics
#if SHOW_DEBUG_METRICS == true
        std::shared_ptr<Message> execMetricsMsg =
                std::make_shared<TransferMetricsMessage>(metrics::TransferMetrics(tupleSet->size(), 0, 0),
                                                         this->name());
        ctx()->notifyRoot(execMetricsMsg);
#endif
      }

      // if having shuffle op
      else {
        std::shared_ptr<Message> tupleSetReadyRemoteMessage =
                std::make_shared<TupleSetReadyRemoteMessage>(host_, flightPort_, true, name_);
        ctx()->tell(tupleSetReadyRemoteMessage);
      }
    }

    // complete
    ctx()->notifyComplete();
  } else {
    // clear shuffled tables at storage if not to be fetched
    if (shufflePOpName_.has_value()) {
      // send request to store
      for (const auto& consumer: consumers_) {
        // send request to store
        auto cmdObj = ClearTableCmd::make(queryId_, name_, consumer);
        auto expCmd = cmdObj->serialize(false);
        if (!expCmd.has_value()) {
          ctx()->notifyError(expCmd.error());
          return;
        }
        auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
        std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
        std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
        status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
        if (!status.ok()) {
          ctx()->notifyError(status.message());
          return;
        }
        status = writer->Close();
        if (!status.ok()) {
          ctx()->notifyError(status.message());
          return;
        }
      }
    }
  }

  // mark double-exec has finished
  if (EnablePushbackTailReqDoubleExec && isDoubleExec) {
    std::unique_lock lock(*doubleExecMutex_);
    isDoubleExecFinished_ = true;
    doubleExecCv_->notify_one();
  }

  // for limiting number of concurrent detached FPDBStoreSuperPOp
  if (ENABLE_FILTER_BITMAP_PUSHDOWN) {
    processDetachOut();
  }
}

void FPDBStoreSuperPOp::processEmpty() {
  // send an empty tupleSet
  std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(TupleSet::makeWithEmptyTable(), name_);
  ctx()->tell(tupleSetMessage);

  // need to clear bitmaps cached at storage if bitmap pushdown is enabled for some ops
  // make flight client and connect
  auto client = flight::GlobalFlightClients.getFlightClient(host_, flightPort_);

  for (const auto &opIt: subPlan_->getPhysicalOps()) {
    auto op = opIt.second;

    // clear bitmap cached at storage for each FilterPOp
    if (op->getType() == POpType::FILTER) {
      auto typedOp = std::static_pointer_cast<filter::FilterPOp>(op);
      if (!typedOp->isBitmapPushdownEnabled()) {
        continue;
      }

      // send request to store
      auto cmdObj = ClearBitmapCmd::make(BitmapType::FILTER_COMPUTE, queryId_, typedOp->getBitmapWrapper()->mirrorOp_);
      auto expCmd = cmdObj->serialize(false);
      if (!expCmd.has_value()) {
        ctx()->notifyError(expCmd.error());
        return;
      }
      auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
      std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
      std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
      auto status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
      if (!status.ok()) {
        ctx()->notifyError(status.message());
        return;
      }
      status = writer->Close();
      if (!status.ok()) {
        ctx()->notifyError(status.message());
        return;
      }
    }
  }

  // complete
  ctx()->notifyComplete();
}

void FPDBStoreSuperPOp::processAsPullup(bool* isResultNeeded) {
  // prepare for pushback double-exec if enabled
  if (EnablePushbackTailReqDoubleExec) {
    std::unique_lock lock(PushbackExecMutex);
    OpsWithPushbackExec[makePushbackDoubleExecKey(queryId_, name_)] = this;
  }

  // get pushback sub-query plan, by changing FPDBFileScanPOp to RemoteFileScanPOp
  // create a "deep" copy of original plan first, by deserialized result
  // FIXME: a better approach is probably to make use of caf's inspect (serialize) mechanism, currently unsure how
  auto expPushbackSubPlan = PhysicalPlanDeserializer::deserialize(*subPlanStr_, "" /*unused*/);
  if (!expPushbackSubPlan.has_value()) {
    ctx()->notifyError(expPushbackSubPlan.error());
    return;
  }
  auto pushbackSubPlan = *expPushbackSubPlan;
  auto res = pushbackSubPlan->fallBackToPullup(host_, fileServicePort_);
  if (!res.has_value()) {
    ctx()->notifyError(res.error());
    return;
  }

  // execute subPlan
  auto execution = std::make_shared<FPDBStoreExecution>(
          queryId_, caf::CAFAdaptPushdownUtil::daemonAdaptPushdownActorSystem_, pushbackSubPlan,
          [&] (const std::string &consumer, const std::shared_ptr<arrow::Table> &table) {
            std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(TupleSet::make(table), name_);
            ctx()->send(tupleSetMessage, consumer);
          },
          [&] (const std::string &, const std::vector<int64_t> &) {
            // noop
          });
  auto tupleSet = execution->execute();

  // ignore the slower output if enabling double-exec
  *isResultNeeded = true;
  if (EnablePushbackTailReqDoubleExec) {
    std::unique_lock lock(*doubleExecMutex_);
    if (isOneExecFinished_) {
      *isResultNeeded = false;
    }
    isOneExecFinished_ = true;
  }

  if (*isResultNeeded) {
    if (tupleSet == nullptr || tupleSet->table() == nullptr) {
      ctx()->notifyError("Received null table from fallback to pullup execution");
      return;
    }
    if (tupleSet->numRows() != 0 || tupleSet->numColumns() != 0) {
      std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(tupleSet, name_);
      ctx()->tell(tupleSetMessage);
    }

    // metrics
#if SHOW_DEBUG_METRICS == true
    std::shared_ptr<Message> execMetricsMsg =
            std::make_shared<TransferMetricsMessage>(execution->getDebugMetrics().getTransferMetrics(), name_);
    ctx()->notifyRoot(execMetricsMsg);
    std::shared_ptr<Message> pushdownFallBackMsg = std::make_shared<PushdownFallBackMessage>(name_);
    ctx()->notifyRoot(pushdownFallBackMsg);
#endif
  }

  // clear unused bloom filters at storage side, due to we fall back to pullup
  // only do when double-exec does not occur
  if (EnablePushbackTailReqDoubleExec) {
    std::unique_lock lock(*doubleExecMutex_);
    if (doubleExecCv_ != nullptr) {
      return;
    }
  }
  std::unordered_set<std::string> bloomFilterKeys;
  std::vector<std::shared_ptr<ClearBitmapCmd>> clearBitmapCmds;
  for (const auto &opIt: subPlan_->getPhysicalOps()) {
    auto op = opIt.second;
    for (const auto &consumerToBloomFilterIt: op->getConsumerToBloomFilterInfo()) {
      const auto &bloomFilterCreatePOp = consumerToBloomFilterIt.second->bloomFilterCreatePOp_;
      auto bloomFilterKey = fmt::format("{}-{}", queryId_, bloomFilterCreatePOp);     // same function of BloomFilterCache::generateBloomFilterKey()
      if (bloomFilterKeys.find(bloomFilterKey) != bloomFilterKeys.end()) {
        continue;
      }
      clearBitmapCmds.emplace_back(
              ClearBitmapCmd::make(BitmapType::BLOOM_FILTER_COMPUTE, queryId_, bloomFilterCreatePOp));
      bloomFilterKeys.emplace(bloomFilterKey);
    }
  }
  // send request to store
  auto client = flight::GlobalFlightClients.getFlightClient(host_, flightPort_);
  for (const auto &cmdObj: clearBitmapCmds) {
    auto expCmd = cmdObj->serialize(false);
    if (!expCmd.has_value()) {
      ctx()->notifyError(expCmd.error());
      return;
    }
    auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
    std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
    std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
    auto status = client->DoPut(descriptor, nullptr, &writer, &metadataReader);
    if (!status.ok()) {
      ctx()->notifyError(status.message());
      return;
    }
    status = writer->Close();
    if (!status.ok()) {
      ctx()->notifyError(status.message());
      return;
    }
  }
}

void FPDBStoreSuperPOp::onErrorDuringProcess(const std::string &error) {
  if (ENABLE_FILTER_BITMAP_PUSHDOWN) {
    processDetachOut();
  }
  ctx()->notifyError(error);
}

tl::expected<std::string, std::string> FPDBStoreSuperPOp::serialize(bool pretty) {
  auto expSubPlanStr = PhysicalPlanSerializer::serialize(subPlan_, pretty);
  if (expSubPlanStr.has_value()) {
    subPlanStr_ = *expSubPlanStr;
  }
  return expSubPlanStr;
}

void FPDBStoreSuperPOp::pushback_double_exec() {
  // start pushdown exec in another thread
  std::thread th(&FPDBStoreSuperPOp::processAtStore, this, true);
  th.detach();
}

void FPDBStoreSuperPOp::clear() {
  subPlan_.reset();
}

}

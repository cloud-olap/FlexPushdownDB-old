//
// Created by Yifei Yang on 3/16/22.
//

#include <fpdb/executor/physical/bloomfilter/BloomFilterCreatePOp.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterCreateKernel.h>
#include <fpdb/executor/physical/bloomfilter/BloomFilterCreateArrowKernel.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/flight/FlightClients.h>
#include <fpdb/store/server/flight/PutBitmapCmd.hpp>
#include <fpdb/tuple/util/Util.h>

using namespace fpdb::store::server::flight;

namespace fpdb::executor::physical::bloomfilter {

BloomFilterCreatePOp::BloomFilterCreatePOp(const std::string &name,
                                           const std::vector<std::string> &projectColumnNames,
                                           int nodeId,
                                           const std::vector<std::string> &bloomFilterColumnNames,
                                           double desiredFalsePositiveRate):
  PhysicalOp(name, BLOOM_FILTER_CREATE, projectColumnNames, nodeId) {
  if (USE_ARROW_BLOOM_FILTER_IMPL) {
    kernel_ = BloomFilterCreateArrowKernel::make(bloomFilterColumnNames);
  } else {
    kernel_ = BloomFilterCreateKernel::make(bloomFilterColumnNames, desiredFalsePositiveRate);
  }
}

std::string BloomFilterCreatePOp::getTypeString() const {
  return "BloomFilterCreatePOp";
}

void BloomFilterCreatePOp::onReceive(const Envelope &envelope) {
  const auto &msg = envelope.message();

  if (msg.type() == MessageType::START) {
    this->onStart();
  } else if (msg.type() == MessageType::TUPLESET) {
    auto tupleSetMessage = dynamic_cast<const TupleSetMessage &>(msg);
    this->onTupleSet(tupleSetMessage);
  } else if (msg.type() == MessageType::COMPLETE) {
    auto completeMessage = dynamic_cast<const CompleteMessage &>(msg);
    this->onComplete(completeMessage);
  } else {
    ctx()->notifyError("Unrecognized message type " + msg.getTypeString());
  }
}

void BloomFilterCreatePOp::produce(const std::shared_ptr<PhysicalOp> &op) {
  passTupleSetConsumers_.emplace(op->name());
  PhysicalOp::produce(op);
}

const std::shared_ptr<BloomFilterCreateAbstractKernel> &BloomFilterCreatePOp::getKernel() const {
  return kernel_;
}

const std::set<std::string> &BloomFilterCreatePOp::getBloomFilterUsePOps() const {
  return bloomFilterUsePOps_;
}

const std::set<std::string> &BloomFilterCreatePOp::getPassTupleSetConsumers() const {
  return passTupleSetConsumers_;
}

void BloomFilterCreatePOp::setBloomFilterUsePOps(const std::set<std::string> &bloomFilterUsePOps) {
  bloomFilterUsePOps_ = bloomFilterUsePOps;
}

void BloomFilterCreatePOp::setPassTupleSetConsumers(const std::set<std::string> &passTupleSetConsumers) {
  passTupleSetConsumers_ = passTupleSetConsumers;
}

void BloomFilterCreatePOp::addBloomFilterUsePOp(const std::shared_ptr<PhysicalOp> &bloomFilterUsePOp) {
  bloomFilterUsePOps_.emplace(bloomFilterUsePOp->name());
  PhysicalOp::produce(bloomFilterUsePOp);
}

void BloomFilterCreatePOp::addFPDBStoreBloomFilterConsumer(
        const std::shared_ptr<PhysicalOp> &fpdbStoreBloomFilterConsumer) {
  fpdbStoreBloomFilterConsumers_.emplace(fpdbStoreBloomFilterConsumer->name());
  PhysicalOp::produce(fpdbStoreBloomFilterConsumer);
}

void BloomFilterCreatePOp::setBloomFilterInfo(const fpdb_store::FPDBStoreBloomFilterCreateInfo &bloomFilterInfo) {
  bloomFilterInfo_ = bloomFilterInfo;
}

void BloomFilterCreatePOp::onStart() {
  SPDLOG_DEBUG("Starting operator  |  name: '{}'", this->name());
}

void BloomFilterCreatePOp::onTupleSet(const TupleSetMessage &msg) {
  // add tupleSet to kernel
  auto tupleSet = msg.tuples();
  auto result = kernel_->bufferTupleSet(tupleSet);
  if (!result.has_value()) {
    ctx()->notifyError(result.error());
  }

  // pass tupleSet to consumers except bloomFilterUsePOps_
  std::shared_ptr<Message> tupleSetMessage = std::make_shared<TupleSetMessage>(tupleSet, name_);
  ctx()->tell(tupleSetMessage, passTupleSetConsumers_);
}

void BloomFilterCreatePOp::onComplete(const CompleteMessage &) {
  if (!ctx()->isComplete() && ctx()->operatorMap().allComplete(POpRelationshipType::Producer)) {
    // build and get bloom filter
    auto res = kernel_->buildBloomFilter();
    if (!res.has_value()) {
      ctx()->notifyError(res.error());
    }
    auto bloomFilter = kernel_->getBloomFilter();
    if (!bloomFilter.has_value()) {
      ctx()->notifyError("Bloom filter not created on complete");
    }

    // send bloom filter to bloomFilterUsePOps_
    std::shared_ptr<Message> bloomFilterMessage = std::make_shared<BloomFilterMessage>(*bloomFilter, name_);
    ctx()->tell(bloomFilterMessage, bloomFilterUsePOps_);

    // send bloom filter to fpdb-store if needed
    if (bloomFilterInfo_.has_value()) {
      putBloomFilterToStore(*bloomFilter);
      notifyFPDBStoreBloomFilterUsers();
    }

    ctx()->notifyComplete();
  }
}

void BloomFilterCreatePOp::putBloomFilterToStore(const std::shared_ptr<BloomFilterBase> &bloomFilter) {
  // bitmap to record batch
  auto expRecordBatches = bloomFilter->makeBitmapRecordBatches();
  if (!expRecordBatches.has_value()) {
    ctx()->notifyError(expRecordBatches.error());
  }
  auto recordBatches = *expRecordBatches;

  // send request to all hosts
  for (const auto &hostIt: bloomFilterInfo_->hosts_) {
    // make flight client and connect
    auto client = flight::GlobalFlightClients.getFlightClient(hostIt.first, bloomFilterInfo_->port_);

    // make flight descriptor
    auto cmdObj = PutBitmapCmd::make(BitmapType::BLOOM_FILTER_COMPUTE, queryId_, name_, true,
                                     hostIt.second, bloomFilter->toJson());
    auto expCmd = cmdObj->serialize(false);
    if (!expCmd.has_value()) {
      ctx()->notifyError(expCmd.error());
    }
    auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);

    // send to host
    std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
    std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
    auto status = client->DoPut(descriptor, recordBatches[0]->schema(), &writer, &metadataReader);
    if (!status.ok()) {
      ctx()->notifyError(status.message());
    }

    for (const auto &batch: recordBatches) {
      status = writer->WriteRecordBatch(*batch);
      if (!status.ok()) {
        ctx()->notifyError(status.message());
      }
    }
    status = writer->DoneWriting();
    if (!status.ok()) {
      ctx()->notifyError(status.message());
    }
    status = writer->Close();
    if (!status.ok()) {
      ctx()->notifyError(status.message());
    }
  }

  // metrics
#if SHOW_DEBUG_METRICS == true
  int64_t recordBatchesSize = 0;
  for (const auto &batch: recordBatches) {
    recordBatchesSize += fpdb::tuple::util::Util::getSize(batch);
  }
  std::shared_ptr<Message> execMetricsMsg = std::make_shared<TransferMetricsMessage>(
          metrics::TransferMetrics(0, recordBatchesSize * bloomFilterInfo_->hosts_.size(), 0), name_);
  ctx()->notifyRoot(execMetricsMsg);
#endif
}

void BloomFilterCreatePOp::notifyFPDBStoreBloomFilterUsers() {
  // just send a msg to notify that bloom filter is ready at store, no real bloom filter in the msg
  std::shared_ptr<Message> bloomFilterMessage = std::make_shared<BloomFilterMessage>(nullptr, name_);
  ctx()->tell(bloomFilterMessage, fpdbStoreBloomFilterConsumers_);
}

void BloomFilterCreatePOp::clear() {
  kernel_->clear();
}

}

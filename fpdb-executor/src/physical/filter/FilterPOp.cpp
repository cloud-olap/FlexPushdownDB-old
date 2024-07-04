//
// Created by matt on 6/5/20.
//

#include <fpdb/executor/physical/filter/FilterPOp.h>
#include <fpdb/executor/physical/fpdb-store/FPDBStoreSuperPOp.h>
#include <fpdb/executor/physical/Globals.h>
#include <fpdb/executor/message/TupleSetMessage.h>
#include <fpdb/executor/message/CompleteMessage.h>
#include <fpdb/executor/message/BitmapMessage.h>
#include <fpdb/executor/message/cache/WeightRequestMessage.h>
#include <fpdb/executor/flight/FlightClients.h>
#include <fpdb/store/server/flight/PutBitmapCmd.hpp>
#include <fpdb/store/server/flight/GetBitmapTicket.hpp>
#include <fpdb/expression/gandiva/And.h>
#include <fpdb/expression/gandiva/Or.h>
#include <fpdb/tuple/Globals.h>
#include <fpdb/tuple/serialization/ArrowSerializer.h>
#include <fpdb/tuple/util/Util.h>
#include <fpdb/util/Util.h>
#include <arrow/flight/api.h>
#include <utility>
#include <cmath>

using namespace fpdb::store::server::flight;
using namespace fpdb::cache;
using namespace fpdb::expression::gandiva;
using namespace fpdb::util;

namespace fpdb::executor::physical::filter {

FilterPOp::FilterPOp(std::string name,
               std::vector<std::string> projectColumnNames,
               int nodeId,
               std::shared_ptr<Expression> predicate,
               std::shared_ptr<Table> table,
               std::vector<std::shared_ptr<SegmentKey>> weightedSegmentKeys) :
	PhysicalOp(std::move(name), FILTER, std::move(projectColumnNames), nodeId),
  predicate_(std::move(predicate)),
  table_(std::move(table)),
	weightedSegmentKeys_(std::move(weightedSegmentKeys)) {}

std::string FilterPOp::getTypeString() const {
  return "FilterPOp";
}

const std::shared_ptr<fpdb::expression::gandiva::Expression> &FilterPOp::getPredicate() const {
  return predicate_;
}

const std::optional<FPDBStoreFilterBitmapWrapper> &FilterPOp::getBitmapWrapper() const {
  return bitmapWrapper_;
}

void FilterPOp::setBitmapWrapper(const FPDBStoreFilterBitmapWrapper &bitmapWrapper) {
  bitmapWrapper_ = bitmapWrapper;
}

bool FilterPOp::isBitmapPushdownEnabled() {
  return bitmapWrapper_.has_value();
}

void FilterPOp::enableBitmapPushdown(const std::string &fpdbStoreSuperPOp,
                                     const std::string &mirrorOp,
                                     bool isComputeSide,
                                     const std::string &host,
                                     int port) {
  bitmapWrapper_ = FPDBStoreFilterBitmapWrapper{};
  bitmapWrapper_->fpdbStoreSuperPOp_ = fpdbStoreSuperPOp;
  bitmapWrapper_->mirrorOp_ = mirrorOp;
  bitmapWrapper_->isComputeSide_ = isComputeSide;
  bitmapWrapper_->host_ = host;
  bitmapWrapper_->port_ = port;
}

void FilterPOp::setBitmap(const std::optional<std::vector<int64_t>> &bitmap) {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }
  bitmapWrapper_->bitmap_ = bitmap;
}

void FilterPOp::onReceive(const Envelope &Envelope) {
  const auto& message = Envelope.message();

  if (message.type() == MessageType::START) {
	  this->onStart();
  } else if (message.type() == MessageType::TUPLESET) {
    auto tupleMessage = dynamic_cast<const TupleSetMessage &>(message);
    this->onTupleSet(tupleMessage);
  } else if (message.type() == MessageType::COMPLETE) {
    auto completeMessage = dynamic_cast<const CompleteMessage &>(message);
    this->onComplete(completeMessage);
  } else {
    ctx()->notifyError("Unrecognized message type " + message.getTypeString());
  }
}

void FilterPOp::onStart() {
  SPDLOG_DEBUG("Starting operator  |  name: '{}'", this->name());
}

void FilterPOp::onTupleSet(const TupleSetMessage &Message) {
  SPDLOG_DEBUG("onTuple  |  Message tupleSet - numRows: {}", Message.tuples()->numRows());
  const auto& tupleSet = Message.tuples();

  // Check applicability, i.e. assign to isApplicable_ and coverSomeProjectColumn_
  checkApplicability(tupleSet);

  // buffer input tupleSet
  bufferReceived(tupleSet);

  // do anything only when the buffer is large enough
  if ((*received_)->numRows() > DefaultBufferSize) {
    if (!isBitmapPushdownEnabled()) {
      onTupleSetRegular();
    } else {
      onTupleSetBitmapPushdown();
    }
  }
}

void FilterPOp::onTupleSetRegular() {
  if (*isApplicable_) {
    // if the filter is applicable, then filter regularly
    buildFilter();
    filterTuplesRegular();
    sendTuples();
  } else {
    // if not applicable, only hybrid execution allows this to happen, where we just send an empty table
    if (isSeparated_) {
      sendEmpty();
      ctx()->notifyComplete();
    } else {
      ctx()->notifyError("Filter predicate is inapplicable to input tupleSet");
    }
  }
}

void FilterPOp::onTupleSetBitmapPushdown() {
  if (isBitmapSet()) {
    // if bitmap is set and it's from storage side, then just use it to filter
    // this should happen at storage side with bitmap sent from compute to storage
    // for the opposite, filter op at compute side will actively fetch bitmap from storage on complete
    filterTuplesUsingBitmap();
    sendTuples();
  } else {
    if (*isApplicable_) {
      // if the filter is applicable, then filter with saving the bitmap
      buildFilter();
      filterTuplesSaveBitMap();
      sendTuples();
    } else {
      if (isComputeSide()) {
        // if at compute side, then send nullopt bitmap to storage side denoting the bitmap cannot be constructed
        // and then wait until receiving bitmap from store
        putBitmapToFPDBStore();
        // unblock associated FPDBStoreSuperPOp
        FPDBStoreSuperPOp::unBlockNonRestrictFilters((*bitmapWrapper_).mirrorOp_, (*bitmapWrapper_).fpdbStoreSuperPOp_);
      } else {
        // filter at storage side should always be applicable
        ctx()->notifyError("Filter predicate is inapplicable to input tupleSet");
      }
    }
  }
}

void FilterPOp::onComplete(const CompleteMessage&) {
  if (!ctx()->isComplete() && ctx()->operatorMap().allComplete(POpRelationshipType::Producer)) {
    if (!isBitmapPushdownEnabled()) {
      onCompleteRegular();
    } else {
      onCompleteBitmapPushdown();
    }
  }
}

void FilterPOp::onCompleteRegular() {
  // filter the rest
  if (received_.has_value()) {
    onTupleSetRegular();

    // if not applicable, then we already complete in the function above
    if (!*isApplicable_) {
      return;
    }
  }

  // send segment weights if required
  if (!weightedSegmentKeys_.empty() && totalNumRows_ > 0 && *isApplicable_) {
    sendSegmentWeight();
  }

  // complete
  ctx()->notifyComplete();
}

void FilterPOp::onCompleteBitmapPushdown() {
  // filter the rest
  if (received_.has_value()) {
    onTupleSetBitmapPushdown();
  }

  if (isComputeSide()) {
    // if at compute side
    if (isBitmapSet()) {
      // if bitmap is set, it means compute side needs to send constructed bitmap to storage side
      putBitmapToFPDBStore();
    } else {
      // filter using bitmap constructed from storage side, only when there is data to filter
      if ((*received_)->numColumns() > 0) {
        // fetch bitmap from storage side
        getBitmapFromFPDBStore();

        // filter using bitmap
        filterTuplesUsingBitmap();
        sendTuples();
      } else {
        sendEmpty();
      }
    }

    // send segment weights if required
    if (!weightedSegmentKeys_.empty() && totalNumRows_ > 0 && *isApplicable_) {
      sendSegmentWeight();
    }

    // complete
    ctx()->notifyComplete();
  } else {
    // if at storage side, put bitmap to flight buffer if bitmap is created during filtering instead of receiving
    // from compute side
    if (filter_.has_value()) {
      sendBitmapToRoot();
    }

    // complete
    ctx()->notifyComplete();
  }
}

void FilterPOp::bufferReceived(const std::shared_ptr<fpdb::tuple::TupleSet>& tupleSet) {
  if (!received_.has_value()) {
    received_ = tupleSet;
  } else {
    auto result = (*received_)->append(tupleSet);
    if (!result.has_value()) {
      ctx()->notifyError(result.error());
    }
  }
}

void FilterPOp::bufferFiltered(const std::shared_ptr<fpdb::tuple::TupleSet>& tupleSet) {
  if (!filtered_.has_value()) {
    filtered_ = tupleSet;
  } else {
    auto result = (*filtered_)->append(tupleSet);
    if (!result.has_value()) {
      ctx()->notifyError(result.error());
    }
  }
}

void FilterPOp::bufferBitMap(const std::shared_ptr<::gandiva::SelectionVector> &selectionVector,
                             int64_t rowOffset,
                             int64_t inputNumRows) {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }

  if (!bitmapWrapper_->bitmap_.has_value()) {
    bitmapWrapper_->bitmap_ = std::vector<int64_t>();
  }
  bitmapWrapper_->bitmap_->resize(std::ceil((double) (rowOffset + inputNumRows) / 64), 0);
  for (int64_t i = 0; i < selectionVector->GetNumSlots(); ++i) {
    setBit(*bitmapWrapper_->bitmap_, selectionVector->GetIndex(i) + rowOffset);
  }
}

std::shared_ptr<::gandiva::SelectionVector> FilterPOp::makeSelectionVector(int64_t startRowOffset, int64_t numRows) {
  if (!isBitmapSet()) {
    ctx()->notifyError("Bitmap not set");
  }

  std::shared_ptr<::gandiva::SelectionVector> selectionVector;
  auto status = ::gandiva::SelectionVector::MakeInt64(numRows, ::arrow::default_memory_pool(), &selectionVector);
  int64_t slotId = 0;
  for (int64_t r = startRowOffset; r < startRowOffset + numRows; ++r) {
    if (getBit(*bitmapWrapper_->bitmap_, r)) {
      selectionVector->SetIndex(slotId++, r - startRowOffset);
    }
  }
  selectionVector->SetNumSlots(slotId);
  return selectionVector;
}

void FilterPOp::buildFilter() {
  if (!filter_.has_value()){
    filter_ = Filter::make(predicate_);

    // input schema and output schema
    auto inputSchema = (*received_)->schema();
    arrow::FieldVector outputFields;
    for (const auto &projectColumnName: getProjectColumnNames()) {
      auto outputField = (*received_)->schema()->GetFieldByName(projectColumnName);
      if (outputField != nullptr) {
        outputFields.emplace_back(outputField);
      }
    }
    auto outputSchema = arrow::schema(outputFields);

    filter_.value()->compile(inputSchema, outputSchema);
  }
}

void FilterPOp::filterTuplesRegular() {
  // metrics
  if (recordSpeeds) {
    totalBytesFiltered_ += (*received_)->size();
  }
  totalNumRows_ += (*received_)->numRows();
  inputBytesFiltered_ += (*received_)->size();
  std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

  // do filter
  const auto &expFiltered = (*filter_)->evaluate(**received_);
  if (!expFiltered.has_value()) {
    ctx()->notifyError(expFiltered.error());
  }
  bufferFiltered(*expFiltered);

  // metrics
  std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now();
  filterTimeNS_ += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
  filteredNumRows_ += (*filtered_)->numRows();
  outputBytesFiltered_ += (*filtered_)->size();

  received_.reset();
}

void FilterPOp::filterTuplesSaveBitMap() {
  int64_t rowOffset = totalNumRows_;

  // metrics
  if (recordSpeeds) {
    totalBytesFiltered_ += (*received_)->size();
  }
  totalNumRows_ += (*received_)->numRows();
  inputBytesFiltered_ += (*received_)->size();
  std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

  // do filter
  std::shared_ptr<::arrow::RecordBatch> batch;
  std::vector<std::shared_ptr<::arrow::RecordBatch>> filteredBatches;
  ::arrow::TableBatchReader reader(*(*received_)->table());

  // Maximum chunk size Gandiva filter evaluates at a time
  reader.set_chunksize((int64_t) fpdb::tuple::DefaultChunkSize);

  auto status = reader.ReadNext(&batch);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }

  while (batch != nullptr) {
    // compute and save bitmap
    auto expSelectionVector = (*filter_)->computeSelectionVector(*batch);
    if (!expSelectionVector.has_value()) {
      ctx()->notifyError(expSelectionVector.error());
    }
    auto selectionVector = *expSelectionVector;
    bufferBitMap(selectionVector, rowOffset, batch->num_rows());

    // project input record batch
    auto projectBatch = TupleSet::projectExist(batch, getProjectColumnNames());

    // get the filtered recordBatch
    auto expFilteredArrays = (*filter_)->evaluateBySelectionVector(*projectBatch, selectionVector);
    if (!expFilteredArrays.has_value()) {
      ctx()->notifyError(expFilteredArrays.error());
    }
    auto filteredBatch = ::arrow::RecordBatch::Make(projectBatch->schema(),
                                                    selectionVector->GetNumSlots(),
                                                    *expFilteredArrays);
    filteredBatches.emplace_back(filteredBatch);

    // next batch
    rowOffset += batch->num_rows();
    status = reader.ReadNext(&batch);
    if (!status.ok()) {
      ctx()->notifyError(status.message());
    }
  }

  // get filtered tupleSet from filtered batches
  auto expFiltered = TupleSet::make(filteredBatches);
  if (!expFiltered) {
    ctx()->notifyError(expFiltered.error());
  }
  bufferFiltered(*expFiltered);

  // metrics
  std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now();
  filterTimeNS_ += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
  filteredNumRows_ += (*filtered_)->numRows();
  outputBytesFiltered_ += (*filtered_)->size();

  received_.reset();
}

void FilterPOp::filterTuplesUsingBitmap() {
  // metrics
  if (recordSpeeds) {
    totalBytesFiltered_ += (*received_)->size();
  }
  totalNumRows_ += (*received_)->numRows();
  inputBytesFiltered_ += (*received_)->size();
  std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

  // do filter
  int64_t rowOffset = 0;
  std::shared_ptr<::arrow::RecordBatch> batch;
  std::vector<std::shared_ptr<::arrow::RecordBatch>> filteredBatches;
  ::arrow::TableBatchReader reader(*(*received_)->table());

  // Maximum chunk size Gandiva filter evaluates at a time
  reader.set_chunksize((int64_t) fpdb::tuple::DefaultChunkSize);

  auto status = reader.ReadNext(&batch);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }

  while (batch != nullptr) {
    // make selection vector from bitmap
    auto selectionVector = makeSelectionVector(rowOffset, batch->num_rows());

    // project input record batch
    auto projectBatch = TupleSet::projectExist(batch, getProjectColumnNames());

    // evaluate the selection vector
    auto expFilteredArrays = Filter::evaluateBySelectionVectorStatic(*projectBatch, selectionVector);
    if (!expFilteredArrays.has_value()) {
      ctx()->notifyError(expFilteredArrays.error());
    }
    auto filteredBatch = ::arrow::RecordBatch::Make(projectBatch->schema(),
                                                    selectionVector->GetNumSlots(),
                                                    *expFilteredArrays);
    filteredBatches.emplace_back(filteredBatch);

    // next batch
    rowOffset += batch->num_rows();
    status = reader.ReadNext(&batch);
    if (!status.ok()) {
      ctx()->notifyError(status.message());
    }
  }

  // get filtered tupleSet from filtered batches
  auto expFiltered = TupleSet::make(filteredBatches);
  if (!expFiltered) {
    ctx()->notifyError(expFiltered.error());
  }
  bufferFiltered(*expFiltered);

  // metrics
  std::chrono::steady_clock::time_point stopTime = std::chrono::steady_clock::now();
  filterTimeNS_ += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
  filteredNumRows_ += (*filtered_)->numRows();
  outputBytesFiltered_ += (*filtered_)->size();

  received_.reset();
}

void FilterPOp::sendTuples() {
  // send, projectExist is already done during filtering
  std::shared_ptr<Message> tupleSetMessage =
          std::make_shared<TupleSetMessage>(*filtered_, name());
  ctx()->tell(tupleSetMessage);

  filtered_.reset();
}

void FilterPOp::sendEmpty() {
  auto emptyTupleSet = fpdb::tuple::TupleSet::makeWithEmptyTable();
  std::shared_ptr<fpdb::executor::message::Message> tupleSetMessage =
          std::make_shared<TupleSetMessage>(emptyTupleSet, name());
  ctx()->tell(tupleSetMessage);
}

int getPredicateNum(const std::shared_ptr<Expression> &expr) {
  if (expr->getType() == AND) {
    auto andExpr = std::static_pointer_cast<And>(expr);
    int cnt = 0;
    for (const auto &subExpr: andExpr->getExprs()) {
      cnt += getPredicateNum(subExpr);
    }
    return cnt;
  } else if (expr->getType() == OR) {
    auto orExpr = std::static_pointer_cast<Or>(expr);
    int cnt = 0;
    for (const auto &subExpr: orExpr->getExprs()) {
      cnt += getPredicateNum(subExpr);
    }
    return cnt;
  } else {
    return 1;
  }
}

void FilterPOp::sendSegmentWeight() {
  /**
   * Weight function:
   *   w = sel / vNetwork + (lenRow / (lenCol * vScan) + #pred / (lenCol * vFilterPOp)) / #key
   */
  auto selectivity = ((double) filteredNumRows_) / ((double ) totalNumRows_);
  auto predicateNum = (double) getPredicateNum(predicate_);
  auto numKey = (double) weightedSegmentKeys_.size();
  std::unordered_map<std::shared_ptr<SegmentKey>, double> weightMap;
  for (auto const &segmentKey: weightedSegmentKeys_) {
    auto columnName = segmentKey->getColumnName();
    auto lenCol = (double) table_->getApxColumnLength(columnName);
    auto lenRow = (double) table_->getApxRowLength();

    auto weight = selectivity / vNetwork + (lenRow / (lenCol * vS3Scan) + predicateNum / (lenCol * vS3Filter)) / numKey;
    weightMap.emplace(segmentKey, weight);
  }

  ctx()->send(WeightRequestMessage::make(weightMap, name()), "SegmentCache");
}

void FilterPOp::sendBitmapToRoot() {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }
  if (!isBitmapSet()) {
    ctx()->notifyError("Bitmap not set when completing at storage side");
  }
  if (bitmapWrapper_->isBitmapSent_) {
    return;
  }

  std::shared_ptr<Message> bitmapMessage = std::make_shared<BitmapMessage>(*bitmapWrapper_->bitmap_, name());
  ctx()->notifyRoot(bitmapMessage);
  bitmapWrapper_->isBitmapSent_ = true;
}

void FilterPOp::putBitmapToFPDBStore() {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }
  if (bitmapWrapper_->isBitmapSent_) {
    return;
  }

  // make flight client and connect
  auto client = flight::GlobalFlightClients.getFlightClient(bitmapWrapper_->host_, bitmapWrapper_->port_);

  // send request to store
  bool valid = isBitmapSet();
  auto bitmap = valid ? *bitmapWrapper_->bitmap_ : std::vector<int64_t>();
  auto expRecordBatch = ArrowSerializer::bitmap_to_recordBatch(bitmap);
  if (!expRecordBatch.has_value()) {
    ctx()->notifyError(expRecordBatch.error());
  }
  auto recordBatch = *expRecordBatch;

  auto cmdObj = PutBitmapCmd::make(BitmapType::FILTER_COMPUTE, queryId_, name_, valid);
  auto expCmd = cmdObj->serialize(false);
  if (!expCmd.has_value()) {
    ctx()->notifyError(expCmd.error());
  }
  auto descriptor = ::arrow::flight::FlightDescriptor::Command(*expCmd);
  std::unique_ptr<arrow::flight::FlightStreamWriter> writer;
  std::unique_ptr<arrow::flight::FlightMetadataReader> metadataReader;
  auto status = client->DoPut(descriptor, recordBatch->schema(), &writer, &metadataReader);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }

  status = writer->WriteRecordBatch(*recordBatch);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }
  status = writer->DoneWriting();
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }
  status = writer->Close();
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }

  // metrics
#if SHOW_DEBUG_METRICS == true
  std::shared_ptr<Message> execMetricsMsg = std::make_shared<TransferMetricsMessage>(
          metrics::TransferMetrics(0, fpdb::tuple::util::Util::getSize(recordBatch), 0), name_);
  ctx()->notifyRoot(execMetricsMsg);
#endif

  bitmapWrapper_->isBitmapSent_ = true;
}

void FilterPOp::getBitmapFromFPDBStore() {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }

  // make flight client and connect
  auto client = flight::GlobalFlightClients.getFlightClient(bitmapWrapper_->host_, bitmapWrapper_->port_);

  // send request to store
  auto ticketObj = GetBitmapTicket::make(queryId_, bitmapWrapper_->mirrorOp_);
  auto expTicket = ticketObj->to_ticket(false);
  if (!expTicket.has_value()) {
    ctx()->notifyError(expTicket.error());
  }

  std::unique_ptr<::arrow::flight::FlightStreamReader> reader;
  auto status = client->DoGet(*expTicket, &reader);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }

  arrow::RecordBatchVector recordBatches;
  status = reader->ReadAll(&recordBatches);
  if (!status.ok()) {
    ctx()->notifyError(status.message());
  }
  if (recordBatches.size() != 1) {
    ctx()->notifyError("Bitmap recordBatch stream should only contain one recordBatch");
  }

  // get bitmap
  auto expBitmap = ArrowSerializer::recordBatch_to_bitmap(recordBatches[0]);
  if (!expBitmap.has_value()) {
    ctx()->notifyError(expBitmap.error());
  }
  bitmapWrapper_->bitmap_ = *expBitmap;

  // metrics
#if SHOW_DEBUG_METRICS == true
  std::shared_ptr<Message> execMetricsMsg = std::make_shared<TransferMetricsMessage>(
          metrics::TransferMetrics(fpdb::tuple::util::Util::getSize(recordBatches[0]), 0, 0), name_);
  ctx()->notifyRoot(execMetricsMsg);
#endif
}

void FilterPOp::checkApplicability(const std::shared_ptr<fpdb::tuple::TupleSet>& tupleSet) {
  if (!isApplicable_.has_value()) {
    auto predicateColumnNames = predicate_->involvedColumnNames();
    std::set<std::string> predicateColumnNameSet(predicateColumnNames.begin(), predicateColumnNames.end());
    auto inputColumnNames = tupleSet->schema()->field_names();
    std::set<std::string> inputColumnNameSet(inputColumnNames.begin(), inputColumnNames.end());
    isApplicable_ = isSubSet(predicateColumnNameSet, inputColumnNameSet);
  }
}

bool FilterPOp::isComputeSide() {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }
  return bitmapWrapper_->isComputeSide_;
}

bool FilterPOp::isBitmapSet() {
  if (!isBitmapPushdownEnabled()) {
    ctx()->notifyError("Bitmap pushdown not enabled");
  }
  return bitmapWrapper_->bitmap_.has_value();
}

size_t FilterPOp::getFilterTimeNS() const {
  return filterTimeNS_;
}

size_t FilterPOp::getFilterInputBytes() const {
  return inputBytesFiltered_;
}

size_t FilterPOp::getFilterOutputBytes() const {
  return outputBytesFiltered_;
}

void FilterPOp::clear() {
  filter_ = std::nullopt;
  received_.reset();
  filtered_.reset();
}

}

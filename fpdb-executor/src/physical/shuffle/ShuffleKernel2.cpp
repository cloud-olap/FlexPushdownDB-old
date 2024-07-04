//
// Created by Yifei Yang on 11/17/22.
//

#include <fpdb/executor/physical/shuffle/ShuffleKernel2.h>
#include <fpdb/tuple/RecordBatchHasher.h>
#include <arrow/compute/api_vector.h>

namespace fpdb::executor::physical::shuffle {

void PartitionRowIdInfo::init(int64_t maxSize) {
  rowIds_ = (int64_t*) malloc(sizeof(int64_t) * maxSize);
  maxSize_ = maxSize;
}

void PartitionRowIdInfo::append(int64_t rowId) {
  rowIds_[currSize_++] = rowId;
  if (currSize_ == maxSize_) {
    maxSize_ *= 2;
    rowIds_ = (int64_t*) realloc(rowIds_, sizeof(int64_t) * maxSize_);
  }
}

void PartitionRowIdInfo::clear() {
  free(rowIds_);
}

tl::expected<std::vector<std::shared_ptr<TupleSet>>, std::string>
ShuffleKernel2::shuffle(const std::vector<std::string> &columnNames,
                        size_t numSlots,
                        const std::shared_ptr<TupleSet> &tupleSet) {
  // make hasher
  auto expHasher = RecordBatchHasher::make(tupleSet->schema(), columnNames);
  if (!expHasher.has_value()) {
    throw std::runtime_error(expHasher.error());
  }
  auto hasher = *expHasher;

  // process for each record batch
  int64_t offset = 0;
  arrow::TableBatchReader reader{*tupleSet->table()};
  auto expRecordBatch = reader.Next();
  if (!expRecordBatch.ok()) {
    return tl::make_unexpected(expRecordBatch.status().message());
  }
  auto recordBatch = *expRecordBatch;

  // allocate for hashes
  uint32_t* hashes = (uint32_t*) malloc(sizeof(uint32_t) * tupleSet->numRows());

  // allocate for partition row ids
  int64_t estMaxPartitionSize = std::max((int64_t) 1, (int64_t) (tupleSet->numRows() / numSlots));
  PartitionRowIdInfo* partitionRowIdInfos = new PartitionRowIdInfo[numSlots];
  for (size_t i = 0; i < numSlots; ++i) {
    partitionRowIdInfos[i].init(estMaxPartitionSize);
  }

  // allocate for merged partition row ids
  int64_t* mergedIndices = (int64_t*) malloc(sizeof(int64_t) * tupleSet->numRows());

  // compute shuffled indices (partition row ids) for each record batch
  while (recordBatch) {
    // compute hashes
    hasher->hash(recordBatch, hashes + offset);

    // save row id to the corresponding partition
    for (int64_t row = offset; row < offset + recordBatch->num_rows(); ++row) {
      partitionRowIdInfos[hashes[row] % numSlots].append(row);
    }

    // next batch
    offset += recordBatch->num_rows();
    expRecordBatch = reader.Next();
    if (!expRecordBatch.ok()) {
      // clear
      clear(numSlots, hashes, partitionRowIdInfos, mergedIndices);
      return tl::make_unexpected(expRecordBatch.status().message());
    }
    recordBatch = *expRecordBatch;
  }

  // call Take() to get the entire shuffled table
  offset = 0;
  for (size_t i = 0; i < numSlots; ++i) {
    memcpy(mergedIndices + offset, partitionRowIdInfos[i].rowIds_, sizeof(int64_t) * partitionRowIdInfos[i].currSize_);
    offset += partitionRowIdInfos[i].currSize_;
  }
  arrow::BufferVector indicesBuffers{2};
  indicesBuffers[0] = nullptr;
  indicesBuffers[1] = std::make_shared<arrow::Buffer>((uint8_t*) mergedIndices, offset);
  auto indicesArrayData = arrow::ArrayData::Make(arrow::int64(), tupleSet->numRows(), indicesBuffers);
  auto indicesArray = std::make_shared<arrow::NumericArray<arrow::Int64Type>>(indicesArrayData);
  auto expTable = arrow::compute::Take(tupleSet->table(), indicesArray);
  if (!expTable.ok()) {
    // clear
    clear(numSlots, hashes, partitionRowIdInfos, mergedIndices);
    return tl::make_unexpected(expTable.status().message());
  }
  auto table = (*expTable).table();

  // slice the entire shuffled table into final shuffled pieces
  std::vector<arrow::ChunkedArrayVector> shuffledColumns{numSlots};
  for (size_t i = 0; i < numSlots; ++i) {
    shuffledColumns[i].resize(table->num_columns());
  }
  for (int c = 0; c < table->num_columns(); ++c) {
    const auto &col = table->column(c);
    offset = 0;
    for (size_t i = 0; i < numSlots; ++i) {
      shuffledColumns[i][c] = col->Slice(offset, partitionRowIdInfos[i].currSize_);
      offset += partitionRowIdInfos[i].currSize_;
    }
  }
  std::vector<std::shared_ptr<TupleSet>> outputTupleSets{numSlots};
  for (size_t i = 0; i < numSlots; ++i) {
    outputTupleSets[i] = TupleSet::make(tupleSet->schema(), shuffledColumns[i]);
  }

  // clear
  clear(numSlots, hashes, partitionRowIdInfos, mergedIndices);
  return outputTupleSets;
}

void ShuffleKernel2::clear(size_t numSlots, uint32_t* hashes,
                           PartitionRowIdInfo* partitionRowIdInfos, int64_t* mergedIndices) {
  free(hashes);
  for (size_t i = 0; i < numSlots; ++i) {
    partitionRowIdInfos[i].clear();
  }
  delete[] partitionRowIdInfos;
  free(mergedIndices);
}

}

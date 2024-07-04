//
// Created by Yifei Yang on 11/17/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_SHUFFLE_SHUFFLEKERNEL2_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_SHUFFLE_SHUFFLEKERNEL2_H

#include <fpdb/tuple/TupleSet.h>
#include <tl/expected.hpp>
#include <vector>
#include <string>

using namespace fpdb::tuple;

namespace fpdb::executor::physical::shuffle {

/**
 * Dynamically allocated memory that stores row ids that are assigned to a single partition.
 */
struct PartitionRowIdInfo {
  int64_t* rowIds_;
  int64_t maxSize_ = 0;
  int64_t currSize_ = 0;

  void init(int64_t maxSize);
  void append(int64_t rowId);
  void clear();
};

class ShuffleKernel2 {
public:
  static tl::expected<std::vector<std::shared_ptr<TupleSet>>, std::string>
  shuffle(const std::vector<std::string> &columnNames,
          size_t numSlots,
          const std::shared_ptr<TupleSet> &tupleSet);

private:
  static void clear(size_t numSlot, uint32_t* hashes,
                    PartitionRowIdInfo* partitionRowIdInfos, int64_t* mergedIndices);
};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_PHYSICAL_SHUFFLE_SHUFFLEKERNEL2_H

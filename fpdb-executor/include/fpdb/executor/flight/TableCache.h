//
// Created by Yifei Yang on 9/28/22.
//

#ifndef FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_TABLECACHE_H
#define FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_TABLECACHE_H

#include <arrow/api.h>
#include <tl/expected.hpp>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <queue>

namespace fpdb::executor::flight {

/**
 * A cache used for tables that will be transferred across the compute cluster using flight.
 */
class TableCache {

public:
  TableCache() = default;

  static std::string generateTableKey(long queryId, const std::string &producer, const std::string &consumer);

  tl::expected<std::shared_ptr<arrow::Table>, std::string> consumeTable(const std::string &key);
  void produceTable(const std::string &key, const std::shared_ptr<arrow::Table> &table);

private:
  std::unordered_map<std::string, std::queue<std::shared_ptr<arrow::Table>>> tables_;
  std::shared_mutex mutex_;

};

}


#endif //FPDB_FPDB_EXECUTOR_INCLUDE_FPDB_EXECUTOR_FLIGHT_TABLECACHE_H

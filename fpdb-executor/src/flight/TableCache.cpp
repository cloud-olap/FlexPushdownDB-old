//
// Created by Yifei Yang on 9/28/22.
//

#include <fpdb/executor/flight/TableCache.h>
#include <fmt/format.h>

namespace fpdb::executor::flight {

std::string TableCache::generateTableKey(long queryId, const std::string &producer, const std::string &consumer) {
  return fmt::format("{}-{}-{}", std::to_string(queryId), producer, consumer);
}

tl::expected<std::shared_ptr<arrow::Table>, std::string> TableCache::consumeTable(const std::string &key) {
  std::unique_lock lock(mutex_);

  auto tableIt = tables_.find(key);
  if (tableIt != tables_.end()) {
    auto& tableQueue = tableIt->second;
    auto table = tableQueue.front();
    tableQueue.pop();
    if (tableQueue.empty()) {
      tables_.erase(tableIt);
    }
    return table;
  } else {
    return tl::make_unexpected(fmt::format("Table with key '{}' not found in the table cache", key));
  }
}

void TableCache::produceTable(const std::string &key, const std::shared_ptr<arrow::Table> &table) {
  std::unique_lock lock(mutex_);

  auto tableIt = tables_.find(key);
  if (tableIt != tables_.end()) {
    tableIt->second.push(table);
  } else {
    std::queue<std::shared_ptr<arrow::Table>> queue;
    queue.push(table);
    tables_[key] = queue;
  }
}

}

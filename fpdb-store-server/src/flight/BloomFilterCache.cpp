//
// Created by Yifei Yang on 9/14/22.
//

#include <fpdb/store/server/flight/BloomFilterCache.hpp>
#include <fmt/format.h>

namespace fpdb::store::server::flight {

std::string BloomFilterCache::generateBloomFilterKey(long queryId, const std::string &op) {
  return fmt::format("{}-{}", std::to_string(queryId), op);
}

tl::expected<std::shared_ptr<BloomFilterBase>, std::string>
BloomFilterCache::consumeBloomFilter(const std::string &key) {
  std::unique_lock lock(mutex_);

  const auto &bloomFilterIt = bloom_filters_.find(key);
  if (bloomFilterIt != bloom_filters_.end()) {
    auto bloomFilter = bloomFilterIt->second.first;
    if (--bloomFilterIt->second.second <= 0) {
      bloom_filters_.erase(bloomFilterIt);
    }
    return bloomFilter;
  } else {
    return tl::make_unexpected(fmt::format("Bloom filter with key '{}' not found in the bloom filter cache", key));
  }
}

void BloomFilterCache::produceBloomFilter(const std::string &key,
                                          const std::shared_ptr<BloomFilterBase> &bloomFilter,
                                          int num_copies) {
  std::unique_lock lock(mutex_);

  bloom_filters_[key] = {bloomFilter, num_copies};
}

}

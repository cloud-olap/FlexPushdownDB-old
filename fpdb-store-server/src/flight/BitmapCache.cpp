//
// Created by Yifei Yang on 4/10/22.
//

#include <fpdb/store/server/flight/BitmapCache.hpp>
#include <fmt/format.h>

namespace fpdb::store::server::flight {

std::string BitmapCache::generateBitmapKey(long queryId, const std::string &op) {
  return fmt::format("{}-{}", std::to_string(queryId), op);
}

tl::expected<std::optional<std::vector<int64_t>>, std::string> BitmapCache::consumeBitmap(const std::string &key) {
  std::unique_lock lock(mutex_);

  auto bitmapIt = bitmaps_.find(key);
  if (bitmapIt != bitmaps_.end()) {
    auto bitmap = bitmapIt->second;
    bitmaps_.erase(bitmapIt);
    return bitmap;
  } else {
    return tl::make_unexpected(fmt::format("Bitmap with key '{}' not found in the bitmap cache", key));
  }
}

void BitmapCache::produceBitmap(const std::string &key, const std::vector<int64_t> &bitmap, bool valid) {
  std::unique_lock lock(mutex_);

  if (valid) {
    bitmaps_[key] = bitmap;
  } else {
    bitmaps_[key] = std::nullopt;
  }
}

}

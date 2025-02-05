//
// Created by Yifei Yang on 9/10/20.
//

#include <fpdb/cache/policy/WLFUCachingPolicy.h>
#include <fmt/format.h>
#include <sstream>
#include <chrono>
#include <utility>

using namespace fpdb::cache::policy;

struct CompValue{
    bool operator()(const std::shared_ptr<SegmentKey> &key1, const std::shared_ptr<SegmentKey> &key2) const{
        return key1->getMetadata()->value() > key2->getMetadata()->value();
    }
};

bool WLFUCachingPolicy::lessValue(const std::shared_ptr<SegmentKey> &key1, const std::shared_ptr<SegmentKey> &key2) {
  return key1->getMetadata()->value() < key2->getMetadata()->value();
}

WLFUCachingPolicy::WLFUCachingPolicy(size_t maxSize,
                                     std::shared_ptr<CatalogueEntry> catalogueEntry):
  CachingPolicy(WLFU,
                maxSize,
                std::move(catalogueEntry),
                true) {}

void WLFUCachingPolicy::onLoad(const std::shared_ptr<SegmentKey> &key) {
    auto startTime = std::chrono::steady_clock::now();
    auto keyEntry = keySet_.find(key);
    if (keyEntry != keySet_.end()) {
        auto realKey = *keyEntry;
        realKey->getMetadata()->incHitNum(getSegmentSize(key));
        if (keySetInCache_.find(key) != keySetInCache_.end()) {
            // allow stale segmentKeys, to avoid to implement increaseKey() in the heap
//            std::make_heap(keysInCache_.begin(), keysInCache_.end(), Comp());
            // make new key
            realKey->getMetadata()->invalidate();
            auto newRealKey = SegmentKey::make(realKey->getPartition(), realKey->getColumnName(), realKey->getRange());
            newRealKey->setMetadata(std::make_shared<SegmentMetadata>(*realKey->getMetadata()));
            // delete
            keySetInCache_.erase(realKey);
            keySet_.erase(realKey);
            // insert
            keysInCache_.push_back(newRealKey);
            std::push_heap(keysInCache_.begin(), keysInCache_.end(), CompValue());
            keySetInCache_.emplace(newRealKey);
            keySet_.emplace(newRealKey);
        }
    } else {
        keySet_.emplace(key);
    }
    auto stopTime = std::chrono::steady_clock::now();
    onLoadTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
}

void WLFUCachingPolicy::onWeight(const std::unordered_map<std::shared_ptr<SegmentKey>, double> &weightMap) {
  // update value using weight
  for (auto const &weightEntry: weightMap) {
    auto segmentKey = weightEntry.first;
    auto weight = weightEntry.second;
    if (weightUpdatedKeys_.find(segmentKey) == weightUpdatedKeys_.end()) {
      std::shared_ptr<SegmentKey> realKey;
      auto keyEntry = keySet_.find(segmentKey);
      if (keyEntry != keySet_.end()) {
        realKey = *keyEntry;
        realKey->getMetadata()->addValue(weight);
      } else {
        throw std::runtime_error("onWeight: Key should exist in keySet_");
      }

      weightUpdatedKeys_.emplace(realKey);
    }
  }
}

void WLFUCachingPolicy::onRemove(const std::shared_ptr<SegmentKey> &key) {
  erase(key);
}

std::optional<std::shared_ptr<std::vector<std::shared_ptr<SegmentKey>>>>
WLFUCachingPolicy::onStore(const std::shared_ptr<SegmentKey> &key) {
    auto startTime = std::chrono::steady_clock::now();

    // decide whether to cache
    std::shared_ptr<SegmentKey> realKey;
    auto keyEntry = keySet_.find(key);
    if (keyEntry != keySet_.end()) {
        realKey = *keyEntry;
        if (realKey->getMetadata()->size() == 0) {
            realKey->getMetadata()->setSize(key->getMetadata()->size());
        }
    } else {
        throw std::runtime_error("onStore: Key should exist in keySet_");
    }

    auto segmentSize = realKey->getMetadata()->size();
    if (maxSize_ < segmentSize) {
        auto stopTime = std::chrono::steady_clock::now();
        onStoreTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
        return std::nullopt;
    }

    size_t tmpFreeSize = freeSize_;
    auto removableKeys = std::make_shared<std::vector<std::shared_ptr<SegmentKey>>>();
    while (tmpFreeSize < segmentSize) {
        auto removableKey = keysInCache_.front();
        if (!removableKey->getMetadata()->valid()) {
            std::pop_heap(keysInCache_.begin(), keysInCache_.end(), CompValue());
            keysInCache_.pop_back();
            continue;
        }
        if (lessValue(removableKey, realKey)) {
            removableKeys->emplace_back(removableKey);
            std::pop_heap(keysInCache_.begin(), keysInCache_.end(), CompValue());
            keysInCache_.pop_back();
            keySetInCache_.erase(removableKey);
            tmpFreeSize += removableKey->getMetadata()->size();
        } else {
            // cannot cache, restore popped keys
            for (auto const &restoreKey: *removableKeys) {
                keysInCache_.push_back(restoreKey);
                std::push_heap(keysInCache_.begin(), keysInCache_.end(), CompValue());
                keySetInCache_.emplace(restoreKey);
            }
            auto stopTime = std::chrono::steady_clock::now();
            onStoreTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
            return std::nullopt;
        }
    }

    // update
    freeSize_ = tmpFreeSize;
    keysInCache_.push_back(realKey);
    std::push_heap(keysInCache_.begin(), keysInCache_.end(), CompValue());
    keySetInCache_.emplace(realKey);
    freeSize_ -= segmentSize;

    auto stopTime = std::chrono::steady_clock::now();
    onStoreTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
    return std::optional(removableKeys);
}

std::shared_ptr<std::vector<std::shared_ptr<SegmentKey>>>
WLFUCachingPolicy::onToCache(std::shared_ptr<std::vector<std::shared_ptr<SegmentKey>>> segmentKeys) {
    auto startTime = std::chrono::steady_clock::now();
    auto keysToCache = std::make_shared<std::vector<std::shared_ptr<SegmentKey>>>();

    // FIXME: an estimation here, if freeSizeOTC_ >= c * maxSize_, we try to cache all segments
    //  Because we cannot know the size of segmentData before bringing it back
    if (freeSizeOTC_ >= maxSize_ * 0.1 && freeSizeOTC_ >= 100*1024*1024) {
        for (auto const &key: *segmentKeys) {
            keysInCacheOTC_.push_back(key);
            std::push_heap(keysInCacheOTC_.begin(), keysInCacheOTC_.end(), CompValue());
            freeSizeOTC_ -= getSegmentSize(key);
        }
        return segmentKeys;
    }

    // estimate whether to cache
    for (auto const &candKey: *segmentKeys) {
        std::shared_ptr<SegmentKey> realKey;
        auto keyEntry = keySet_.find(candKey);
        // keys have been added to keySet_ in onLoad() before
        if (keyEntry != keySet_.end()) {
            realKey = *keyEntry;
        } else {
            throw std::runtime_error("onToCache: Key should exist in keySet_");
        }

        size_t tmpFreeSizeOTC = freeSizeOTC_;
        auto segmentSize = getSegmentSize(realKey);
        std::vector<std::shared_ptr<SegmentKey>> removableKeys;
        bool toCache = true;
        while (tmpFreeSizeOTC < segmentSize) {
            auto removableKey = keysInCacheOTC_.front();
            if (!removableKey->getMetadata()->valid()) {
                std::pop_heap(keysInCacheOTC_.begin(), keysInCacheOTC_.end(), CompValue());
                keysInCacheOTC_.pop_back();
                continue;
            }
            if (lessValue(removableKey, realKey)) {
                removableKeys.emplace_back(removableKey);
                std::pop_heap(keysInCacheOTC_.begin(), keysInCacheOTC_.end(), CompValue());
                keysInCacheOTC_.pop_back();
                tmpFreeSizeOTC += getSegmentSize(removableKey);
            } else {
                // not to cache, restore popped keys
                for (auto const &key: removableKeys) {
                    keysInCacheOTC_.push_back(key);
                    std::push_heap(keysInCacheOTC_.begin(), keysInCacheOTC_.end(), CompValue());
                }
                toCache = false;
                break;
            }
        }

        if (toCache) {
            freeSizeOTC_ = tmpFreeSizeOTC;
            keysInCacheOTC_.push_back(realKey);
            std::push_heap(keysInCacheOTC_.begin(), keysInCacheOTC_.end(), CompValue());
            freeSizeOTC_ -= segmentSize;
            keysToCache->emplace_back(realKey);
        }
    }

    auto stopTime = std::chrono::steady_clock::now();
    onToCacheTime += std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime).count();
    return keysToCache;
}

void WLFUCachingPolicy::erase(const std::shared_ptr<SegmentKey> &key) {
  keysInCache_.erase(std::find(keysInCache_.begin(), keysInCache_.end(), key));
}

std::shared_ptr<std::unordered_set<std::shared_ptr<SegmentKey>, SegmentKeyPointerHash, SegmentKeyPointerPredicate>>
WLFUCachingPolicy::getKeysetInCachePolicy() {
  auto keysetInCachePolicy = std::make_shared<std::unordered_set<std::shared_ptr<SegmentKey>, SegmentKeyPointerHash, SegmentKeyPointerPredicate>>();
  keysetInCachePolicy->insert(keysInCache_.begin(), keysInCache_.end());
  return keysetInCachePolicy;
}

std::string WLFUCachingPolicy::showCurrentLayout() {
  std::stringstream ss;
  ss << "Total numbers: " << keysInCache_.size() << std::endl;
  for (auto const &segmentKey: keysInCache_) {
    ss << fmt::format("Key: {};\tHitNum: {}\tValue: {}",
                      segmentKey->toString(),
                      segmentKey->getMetadata()->hitNum(),
                      segmentKey->getMetadata()->value())
      << std::endl;
  }
  ss << "Max size: " << maxSize_ << std::endl;
  ss << "Free size: " << freeSize_ << std::endl;
  return ss.str();
}

std::string WLFUCachingPolicy::toString() {
  return "W-LFU";
}

void WLFUCachingPolicy::onNewQuery() {
  weightUpdatedKeys_.clear();
  freeSizeOTC_ = freeSize_;
  keysInCacheOTC_.assign(keysInCache_.begin(), keysInCache_.end());
}

void WLFUCachingPolicy::onClear() {
  keysInCache_.clear();
  keySetInCache_.clear();
  keySet_.clear();
  freeSizeOTC_ = 0;
  keysInCacheOTC_.clear();
  weightUpdatedKeys_.clear();
}

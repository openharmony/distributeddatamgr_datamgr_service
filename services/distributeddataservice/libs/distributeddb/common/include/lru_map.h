/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LRU_MAP_H
#define LRU_MAP_H
#include <map>
#include <mutex>
#include <queue>

#include "db_errno.h"
#include "macro_utils.h"

namespace DistributedDB {
// LRU map
template<typename K, typename V>
class LruMap final {
public:
    LruMap() = default;
    ~LruMap() = default;

    DISABLE_COPY_ASSIGN_MOVE(LruMap);

    int Get(const K &key, V &outValue)
    {
        std::lock_guard<std::mutex> autoLock(lruLock_);
        if (cache_.find(key) == cache_.end()) {
            return -E_NOT_FOUND;
        }
        outValue = cache_[key];
        return Elimination(key, outValue);
    }

    int Put(const K &key, const V &inValue)
    {
        std::lock_guard<std::mutex> autoLock(lruLock_);
        cache_[key] = inValue;
        return Elimination(key, inValue);
    }

    void RemoveWithPrefixKey(const K &prefixKey)
    {
        std::lock_guard<std::mutex> autoLock(lruLock_);
        auto iterator = eliminationChain_.begin();
        while (iterator != eliminationChain_.end()) {
            const K &key = (*iterator).first;
            if (key.find(prefixKey) == 0) {
                (void)cache_.erase(key);
                iterator = eliminationChain_.erase(iterator);
            } else {
                iterator++;
            }
        }
    }

private:
    // move the node to last and remove the first node until the size less than limit
    int Elimination(const K &key, const V &val)
    {
        auto iterator = eliminationChain_.begin();
        while (iterator != eliminationChain_.end()) {
            if ((*iterator).first == key) {
                eliminationChain_.erase(iterator);
                break;
            }
            iterator++;
        }
        std::pair<K, V> entry = {key, val};
        eliminationChain_.push_back(entry);
        while (eliminationChain_.size() > MAX_CACHE_ITEMS) {
            std::pair<K, V> &pair = eliminationChain_.front();
            cache_.erase(pair.first);
            eliminationChain_.pop_front();
        }
        return E_OK;
    }

    std::mutex lruLock_;
    std::map<K, V> cache_;
    std::deque<std::pair<K, V>> eliminationChain_;

    static const int MAX_CACHE_ITEMS = 200;
};
} // namespace DistributedDB
#endif // LRU_MAP_H
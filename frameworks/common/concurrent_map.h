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

#ifndef DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_MAP_H
#define DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_MAP_H

#include <map>
#include <mutex>
#include <functional>

namespace OHOS {
template<typename Key, typename Tp>
class ConcurrentMap {
public:
    using key_type = typename std::map<Key, Tp>::key_type;
    using mapped_type = typename std::map<Key, Tp>::mapped_type;
    using value_type = typename std::map<Key, Tp>::value_type;
    using size_type = typename std::map<Key, Tp>::size_type;
    using reference = typename std::map<Key, Tp>::reference;
    using const_reference = typename std::map<Key, Tp>::const_reference;
    using Predicate = std::function<bool(const key_type&, const mapped_type&)>;
    
    ConcurrentMap() = default;
    ~ConcurrentMap() = default;
    ConcurrentMap(const ConcurrentMap &other)
    {
        operator=(std::move(other));
    }
    
    ConcurrentMap &operator=(const ConcurrentMap &other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        auto tmp = other.Clone();
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        entries_ = std::move(tmp);
        return *this;
    }
    
    ConcurrentMap(ConcurrentMap &&other) noexcept
    {
        operator=(std::move(other));
    }
    
    ConcurrentMap &operator=(ConcurrentMap &&other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        auto tmp = other.Steal();
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        entries_ = std::move(tmp);
        return *this;
    }
    
    template<typename... _Args>
    bool Emplace(_Args &&...__args) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.emplace(std::forward<_Args>(__args)...);
        return it->second;
    }
    
    std::pair<bool, mapped_type> Find(const key_type &key) const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return std::pair { false, mapped_type() };
        }
        
        return std::pair { true, it->second };
    }
    
    bool Contains(const key_type& key) const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return (entries_.find(key) != entries_.end());
    }
    
    bool Insert(const key_type &key, const mapped_type &value) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.insert(value_type{ key, value });
        return it.second;
    }
    
    size_type Erase(const key_type &key) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.erase(key);
    }
    
    size_type EraseAllIf(const Predicate& predicate) noexcept
    {
        size_type count = 0;
        if (predicate == nullptr) {
            return 0;
        }
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (predicate(it->first, it->second)) {
                entries_.erase(it++);
                count++;
                continue;
            }
            it++;
        }
        return count;
    }
    
    void Clear() noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.clear();
    }
    
    bool Empty() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.empty();
    }
    
    size_type Size() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.size();
    }
    
    void ClearIf(std::function<bool(const key_type &key, mapped_type &value)> &action) noexcept
    {
        if (action == nullptr) {
            return;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        for (auto &[key, value] : entries_) {
            if (action(key, value)) {
                break;
            }
        }
        return entries_.size();
    }
    
    mapped_type &operator[](const key_type &key) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_[key];
    }
    
    const mapped_type &operator[](const key_type &key) const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_[key];
    }
    
    void ForEach(const std::function<bool(const key_type &, mapped_type &)> &action)
    {
        if (action == nullptr) {
            return;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        for (auto &[key, value] : entries_) {
            if (action(key, value)) {
                break;
            }
        }
    }
    
    bool Compute(const key_type &key, const std::function<bool(const key_type &, mapped_type &)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            auto result = entries_.emplace(key, mapped_type());
            it = result.second ? result.first : entries_.end();
        }
        if (it == entries_.end()) {
            return false;
        }
        action(it->first, it->second);
        return true;
    }
    
    bool ComputeIfPresent(const key_type &key, const std::function<void(const key_type &, mapped_type &)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return false;
        }
        action(key, it->second);
        return true;
    }
    
    mapped_type ComputeIfAbsent(const key_type &key, const std::function<mapped_type(const key_type &)> &action)
    {
        if (action == nullptr) {
            return mapped_type();
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            return it->second;
        }
        mapped_type value = action(key);
        entries_.insert(value_type{ key, value });
        return value;
    }
private:
    std::map<Key, Tp> Steal() noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return std::move(entries_);
    }
    
    std::map<Key, Tp> Clone() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_;
    }
private:
    mutable std::recursive_mutex mutex_;
    std::map<Key, Tp> entries_;
};
}
#endif

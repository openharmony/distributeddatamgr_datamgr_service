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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_MAP_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_MAP_H
#include <functional>
#include <map>
#include <mutex>
namespace OHOS {
template<typename _Key, typename _Tp>
class ConcurrentMap {
public:
    using key_type = typename std::map<_Key, _Tp>::key_type;
    using mapped_type = typename std::map<_Key, _Tp>::mapped_type;
    using value_type = typename std::map<_Key, _Tp>::value_type;
    using size_type = typename std::map<_Key, _Tp>::size_type;
    using reference = typename std::map<_Key, _Tp>::reference;
    using const_reference = typename std::map<_Key, _Tp>::const_reference;

    ConcurrentMap() = default;
    ~ConcurrentMap()
    {
        Clear();
    }

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

    bool Contains(const key_type &key) const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return (entries_.find(key) != entries_.end());
    }

    template <typename _Obj>
    bool InsertOrAssign(const key_type &key, _Obj &&obj) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.insert_or_assign(key, std::forward<_Obj>(obj));
        return it.second;
    }

    bool Insert(const key_type &key, const mapped_type &value) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.insert(value_type { key, value });
        return it.second;
    }

    size_type Erase(const key_type &key) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.erase(key);
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

    // The action`s return true mains meet the erase condition
    // The action`s return false mains not meet the erase condition
    size_type EraseIf(const std::function<bool(const key_type &key, mapped_type &value)> &action) noexcept
    {
        if (action == nullptr) {
            return 0;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
#if __cplusplus > 201703L
        auto count = std::erase_if(entries_,
            [&action](value_type &value) -> bool { return action(value.first, value.second); });
#else
        auto count = entries_.size();
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (action((*it).first, (*it).second)) {
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
        count -= entries_.size();
#endif
        return count;
    }

    mapped_type &operator[](const key_type &key) noexcept
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

    // The action's return value mains that the element is keep in map or not; true mains keep, false mains remove.
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
        if (!action(it->first, it->second)) {
            entries_.erase(key);
        }
        return true;
    }

    // The action's return value mains that the element is keep in map or not; true mains keep, false mains remove.
    bool ComputeIfPresent(const key_type &key, const std::function<bool(const key_type &, mapped_type &)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return false;
        }
        if (!action(key, it->second)) {
            entries_.erase(key);
        }
        return true;
    }

    bool ComputeIfAbsent(const key_type &key, const std::function<mapped_type(const key_type &)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            return false;
        }
        entries_.emplace(key, action(key));
        return true;
    }

private:
    std::map<_Key, _Tp> Steal() noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return std::move(entries_);
    }

    std::map<_Key, _Tp> Clone() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_;
    }

private:
    mutable std::recursive_mutex mutex_;
    std::map<_Key, _Tp> entries_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_MAP_H

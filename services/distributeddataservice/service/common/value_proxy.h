/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_COMMON_VALUE_PROXY_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_COMMON_VALUE_PROXY_H
#include "asset_value.h"
#include "cloud/cloud_store_types.h"
#include "distributeddb/result_set.h"
#include "store/general_value.h"
#include "value_object.h"
#include "values_bucket.h"
#include "common_types.h"

namespace OHOS::DistributedData {
class ValueProxy final {
public:
    template<class T>
    static inline constexpr uint32_t INDEX = DistributedData::TYPE_INDEX<T>;
    static inline constexpr uint32_t MAX = DistributedData::TYPE_MAX;

    template<typename T, typename... Types>
    struct variant_cvt_of {
        static constexpr size_t value = std::is_class_v<T> ? Traits::convertible_index_of_v<T, Types...>
                                                           : Traits::same_index_of_v<T, Types...>;
    };

    template<typename T, typename... Types>
    static variant_cvt_of<T, Types...> variant_cvt_test(const T &, const std::variant<Types...> &);

    template<class T, class V>
    static inline constexpr uint32_t CVT_INDEX =
        decltype(variant_cvt_test(std::declval<T>(), std::declval<V>()))::value;

    using Bytes = DistributedData::Bytes;
    class Asset {
    public:
        Asset() = default;
        Asset(Asset &&proxy) noexcept
        {
            *this = std::move(proxy);
        };
        Asset(const Asset &proxy)
        {
            *this = proxy;
        };
        Asset(DistributedData::Asset asset);
        Asset(CommonType::AssetValue asset);
        Asset(NativeRdb::AssetValue asset);
        Asset(DistributedDB::Asset asset);
        Asset &operator=(const Asset &proxy);
        Asset &operator=(Asset &&proxy) noexcept;
        operator CommonType::AssetValue();
        operator NativeRdb::AssetValue();
        operator DistributedData::Asset();
        operator DistributedDB::Asset();
        static uint32_t ConvertToDataStatus(const DistributedDB::Asset &asset);
        static uint32_t ConvertToDBStatus(const uint32_t &status);

    private:
        DistributedData::Asset asset_;
    };

    class TempAsset {
    public:
        explicit TempAsset(DistributedDB::Asset asset);
        operator NativeRdb::AssetValue();

    private:
        static uint32_t ConvertToDataStatus(const uint32_t &status);
        DistributedData::Asset asset_;
    };

    class Assets {
    public:
        Assets() = default;
        Assets(Assets &&proxy) noexcept
        {
            *this = std::move(proxy);
        };
        Assets(const Assets &proxy)
        {
            *this = proxy;
        };
        Assets(DistributedData::Assets assets);
        Assets(CommonType::Assets assets);
        Assets(NativeRdb::ValueObject::Assets assets);
        Assets(DistributedDB::Assets assets);
        Assets &operator=(const Assets &proxy);
        Assets &operator=(Assets &&proxy) noexcept;
        operator CommonType::Assets();
        operator NativeRdb::ValueObject::Assets();
        operator DistributedData::Assets();
        operator DistributedDB::Assets();

    private:
        std::vector<Asset> assets_;
    };
    using Relations = std::map<std::string, std::string>;
    using Proxy = std::variant<std::monostate, int64_t, double, std::string, bool, Bytes, Asset, Assets, Relations>;

    class Value {
    public:
        Value() = default;
        Value(Value &&value) noexcept
        {
            *this = std::move(value);
        };
        Value &operator=(Value &&value) noexcept;
        operator NativeRdb::ValueObject();
        operator CommonType::Value();
        operator DistributedData::Value();
        operator DistributedDB::Type();

        template<typename T>
        operator T() noexcept
        {
            auto val = Traits::get_if<T>(&value_);
            if (val != nullptr) {
                return T(std::move(*val));
            }
            return T();
        };

    private:
        friend ValueProxy;
        Proxy value_;
    };
    class Values {
    public:
        Values() = default;
        Values(Values &&values) noexcept
        {
            *this = std::move(values);
        };
        Values &operator=(Values &&values) noexcept;
        template<typename T>
        operator std::vector<T>()
        {
            std::vector<T> objects;
            objects.reserve(value_.size());
            for (auto &proxy : value_) {
                objects.emplace_back(std::move(proxy));
            }
            value_.clear();
            return objects;
        }

    private:
        friend ValueProxy;
        std::vector<Value> value_;
    };
    class Bucket {
    public:
        Bucket() = default;
        Bucket(Bucket &&bucket) noexcept
        {
            *this = std::move(bucket);
        };
        Bucket &operator=(Bucket &&bucket) noexcept;
        template<typename T>
        operator std::map<std::string, T>()
        {
            std::map<std::string, T> bucket;
            for (auto &[key, value] : value_) {
                bucket.insert_or_assign(key, std::move(static_cast<T>(value)));
            }
            value_.clear();
            return bucket;
        }
        operator NativeRdb::ValuesBucket();
        operator CommonType::ValuesBucket();

    private:
        friend ValueProxy;
        std::map<std::string, Value> value_;
    };
    class Buckets {
    public:
        Buckets() = default;
        Buckets(Buckets &&buckets) noexcept
        {
            *this = std::move(buckets);
        };
        Buckets &operator=(Buckets &&buckets) noexcept;
        template<typename T>
        operator std::vector<T>()
        {
            std::vector<T> buckets;
            buckets.reserve(value_.size());
            for (auto &bucket : value_) {
                buckets.emplace_back(std::move(bucket));
            }
            value_.clear();
            return buckets;
        }

    private:
        friend ValueProxy;
        std::vector<Bucket> value_;
    };

    static Value Convert(DistributedData::Value &&value);
    static Value Convert(CommonType::Value &&value);
    static Value Convert(NativeRdb::ValueObject &&value);
    static Value Convert(DistributedDB::Type &&value);
    static Values Convert(DistributedData::Values &&values);
    static Values Convert(std::vector<CommonType::Value> &&values);
    static Values Convert(std::vector<NativeRdb::ValueObject> &&values);
    static Values Convert(std::vector<DistributedDB::Type> &&values);
    static Bucket Convert(DistributedData::VBucket &&bucket);
    static Bucket Convert(NativeRdb::ValuesBucket &&bucket);
    static Bucket Convert(CommonType::ValuesBucket &&bucket);
    static Bucket Convert(DistributedDB::VBucket &&bucket);
    static Buckets Convert(DistributedData::VBuckets &&buckets);
    static Buckets Convert(std::vector<NativeRdb::ValuesBucket> &&buckets);
    static Buckets Convert(std::vector<CommonType::ValuesBucket> &&buckets);
    static Buckets Convert(std::vector<DistributedDB::VBucket> &&buckets);

    static Value Convert(DistributedDB::VariantData &&value);
    static Bucket Convert(std::map<std::string, DistributedDB::VariantData> &&value);

    template<typename T>
    static std::enable_if_t < CVT_INDEX<T, Proxy><MAX, Bucket>
    Convert(const std::map<std::string, T> &values)
    {
        Bucket bucket;
        for (auto &[key, value] : values) {
            bucket.value_[key].value_ = static_cast<std::variant_alternative_t<CVT_INDEX<T, Proxy>, Proxy>>(value);
        }
        return bucket;
    }

    template<typename T>
    static std::enable_if_t < CVT_INDEX<T, Proxy><MAX, Values>
    Convert(const std::vector<T> &values)
    {
        Values proxy;
        proxy.value_.resize(values.size());
        for (size_t i = 0; i < values.size(); i++) {
            proxy.value_[i].value_ = static_cast<std::variant_alternative_t<CVT_INDEX<T, Proxy>, Proxy>>(values[i]);
        }
        return proxy;
    }

private:
    ValueProxy() = delete;
    ~ValueProxy() = delete;
    static inline uint32_t GetHighStatus(uint32_t status)
    {
        return status & 0xF0000000;
    }

    static inline uint32_t GetLowStatus(uint32_t status)
    {
        return status & ~0xF0000000;
    }
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_COMMON_VALUE_PROXY_H

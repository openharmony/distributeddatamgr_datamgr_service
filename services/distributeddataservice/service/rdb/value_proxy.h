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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_VALUE_PROXY_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_VALUE_PROXY_H
#include "asset_value.h"
#include "store/general_value.h"
#include "value_object.h"
#include "values_bucket.h"
namespace OHOS::DistributedRdb {
class ValueProxy final {
public:
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
        Asset(NativeRdb::AssetValue asset);
        Asset &operator=(const Asset &proxy);
        Asset &operator=(Asset &&proxy) noexcept;
        operator NativeRdb::AssetValue();
        operator DistributedData::Asset();

    private:
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
        Assets(NativeRdb::ValueObject::Assets assets);
        Assets &operator=(const Assets &proxy);
        Assets &operator=(Assets &&proxy) noexcept;
        operator NativeRdb::ValueObject::Assets();
        operator DistributedData::Assets();

    private:
        std::vector<Asset> assets_;
    };
    using Proxy = std::variant<std::monostate, int64_t, double, std::string, bool, Bytes, Asset, Assets>;

    class Value {
    public:
        Value() = default;
        Value(Value &&value) noexcept
        {
            *this = std::move(value);
        };
        Value &operator=(Value &&value) noexcept;
        operator NativeRdb::ValueObject();
        operator DistributedData::Value();

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
                bucket.insert_or_assign(key, std::move(value));
            }
            value_.clear();
            return bucket;
        }
        operator NativeRdb::ValuesBucket();

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
    static Value Convert(NativeRdb::ValueObject &&value);
    static Values Convert(DistributedData::Values &&values);
    static Values Convert(std::vector<NativeRdb::ValueObject> &&values);
    static Bucket Convert(DistributedData::VBucket &&bucket);
    static Bucket Convert(NativeRdb::ValuesBucket &&bucket);
    static Buckets Convert(DistributedData::VBuckets &&buckets);
    static Buckets Convert(std::vector<NativeRdb::ValuesBucket> &&buckets);

private:
    ValueProxy() = delete;
    ~ValueProxy() = delete;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_VALUE_PROXY_H

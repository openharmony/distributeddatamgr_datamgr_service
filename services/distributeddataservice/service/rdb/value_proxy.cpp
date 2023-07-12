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

#define LOG_TAG "ValueProxy"
#include "value_proxy.h"
namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
ValueProxy::Value ValueProxy::Convert(DistributedData::Value &&value)
{
    Value proxy;
    DistributedData::Convert(std::move(value), proxy.value_);
    return proxy;
}

ValueProxy::Value ValueProxy::Convert(NativeRdb::ValueObject &&value)
{
    Value proxy;
    DistributedData::Convert(std::move(value.value), proxy.value_);
    return proxy;
}

ValueProxy::Value ValueProxy::Convert(DistributedDB::Type &&value)
{
    Value proxy;
    DistributedData::Convert(std::move(value), proxy.value_);
    return proxy;
}

ValueProxy::Values ValueProxy::Convert(DistributedData::Values &&values)
{
    Values proxy;
    proxy.value_.reserve(values.size());
    for (auto &value : values) {
        proxy.value_.emplace_back(Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Values ValueProxy::Convert(std::vector<NativeRdb::ValueObject> &&values)
{
    Values proxy;
    proxy.value_.reserve(values.size());
    for (auto &value : values) {
        proxy.value_.emplace_back(Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Bucket ValueProxy::Convert(DistributedData::VBucket &&bucket)
{
    ValueProxy::Bucket proxy;
    for (auto &[key, value] : bucket) {
        proxy.value_.insert_or_assign(key, Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Bucket ValueProxy::Convert(NativeRdb::ValuesBucket &&bucket)
{
    ValueProxy::Bucket proxy;
    for (auto &[key, value] : bucket.values_) {
        proxy.value_.insert_or_assign(key, Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Bucket ValueProxy::Convert(DistributedDB::VBucket &&bucket)
{
    ValueProxy::Bucket proxy;
    for (auto &[key, value] : bucket) {
        proxy.value_.insert_or_assign(key, Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Buckets ValueProxy::Convert(std::vector<NativeRdb::ValuesBucket> &&buckets)
{
    ValueProxy::Buckets proxy;
    proxy.value_.reserve(buckets.size());
    for (auto &bucket : buckets) {
        proxy.value_.emplace_back(Convert(std::move(bucket)));
    }
    return proxy;
}

ValueProxy::Buckets ValueProxy::Convert(std::vector<DistributedDB::VBucket> &&buckets)
{
    ValueProxy::Buckets proxy;
    proxy.value_.reserve(buckets.size());
    for (auto &bucket : buckets) {
        proxy.value_.emplace_back(Convert(std::move(bucket)));
    }
    return proxy;
}

ValueProxy::Buckets ValueProxy::Convert(DistributedData::VBuckets &&buckets)
{
    ValueProxy::Buckets proxy;
    proxy.value_.reserve(buckets.size());
    for (auto &bucket : buckets) {
        proxy.value_.emplace_back(Convert(std::move(bucket)));
    }
    return proxy;
}

ValueProxy::Value ValueProxy::Convert(DistributedDB::VariantData &&value)
{
    Value proxy;
    DistributedData::Convert(std::move(value), proxy.value_);
    return proxy;
}

ValueProxy::Bucket ValueProxy::Convert(std::map<std::string, DistributedDB::VariantData> &&value)
{
    ValueProxy::Bucket proxy;
    for (auto &[key, value] : value) {
        proxy.value_.insert_or_assign(key, Convert(std::move(value)));
    }
    return proxy;
}

ValueProxy::Asset::Asset(DistributedData::Asset asset)
{
    asset_ = std::move(asset);
}

ValueProxy::Asset::Asset(NativeRdb::AssetValue asset)
{
    asset_ = DistributedData::Asset { .version = asset.version,
        .status = asset.status,
        .expiresTime = asset.expiresTime,
        .id = std::move(asset.id),
        .name = std::move(asset.name),
        .uri = std::move(asset.uri),
        .createTime = std::move(asset.createTime),
        .modifyTime = std::move(asset.modifyTime),
        .size = std::move(asset.size),
        .hash = std::move(asset.hash),
        .path = std::move(asset.path) };
}

ValueProxy::Asset::Asset(DistributedDB::Asset asset)
{
    asset_ = DistributedData::Asset { .version = asset.version,
        .status = ConvertToDataStatus(asset),
        .expiresTime = DistributedData::Asset::NO_EXPIRES_TIME,
        .id = std::move(asset.assetId),
        .name = std::move(asset.name),
        .uri = std::move(asset.uri),
        .createTime = std::move(asset.createTime),
        .modifyTime = std::move(asset.modifyTime),
        .size = std::move(asset.size),
        .hash = std::move(asset.hash),
        .path = std::move(asset.subpath) };
}

ValueProxy::Asset &ValueProxy::Asset::operator=(const Asset &proxy)
{
    if (this == &proxy) {
        return *this;
    }
    asset_ = proxy.asset_;
    return *this;
}

ValueProxy::Asset &ValueProxy::Asset::operator=(Asset &&proxy) noexcept
{
    if (this == &proxy) {
        return *this;
    }
    asset_ = std::move(proxy);
    return *this;
}

ValueProxy::Asset::operator NativeRdb::AssetValue()
{
    return NativeRdb::AssetValue { .version = asset_.version,
        .status = asset_.status,
        .expiresTime = asset_.expiresTime,
        .id = std::move(asset_.id),
        .name = std::move(asset_.name),
        .uri = std::move(asset_.uri),
        .createTime = std::move(asset_.createTime),
        .modifyTime = std::move(asset_.modifyTime),
        .size = std::move(asset_.size),
        .hash = std::move(asset_.hash),
        .path = std::move(asset_.path) };
}

ValueProxy::Asset::operator DistributedData::Asset()
{
    return std::move(asset_);
}

ValueProxy::Asset::operator DistributedDB::Asset()
{
    return DistributedDB::Asset { .version = asset_.version,
        .name = std::move(asset_.name),
        .assetId = std::move(asset_.id),
        .subpath = std::move(asset_.path),
        .uri = std::move(asset_.uri),
        .modifyTime = std::move(asset_.modifyTime),
        .createTime = std::move(asset_.createTime),
        .size = std::move(asset_.size),
        .hash = std::move(asset_.hash),
        .status = ConvertToDBStatus(asset_) };
}

uint32_t ValueProxy::Asset::ConvertToDataStatus(const DistributedDB::Asset &asset)
{
    if (asset.status == DistributedDB::AssetStatus::DOWNLOADING) {
        if (asset.flag == static_cast<uint32_t>(DistributedDB::AssetOpType::DELETE)) {
            return DistributedData::Asset::STATUS_DELETE;
        }
        return DistributedData::Asset::STATUS_DOWNLOADING;
    } else if (asset.status == DistributedDB::AssetStatus::ABNORMAL) {
        return DistributedData::Asset::STATUS_ABNORMAL;
    } else {
        switch (asset.flag) {
            case static_cast<uint32_t>(DistributedDB::AssetOpType::INSERT):
                return DistributedData::Asset::STATUS_INSERT;
            case static_cast<uint32_t>(DistributedDB::AssetOpType::UPDATE):
                return DistributedData::Asset::STATUS_UPDATE;
            case static_cast<uint32_t>(DistributedDB::AssetOpType::DELETE):
                return DistributedData::Asset::STATUS_DELETE;
            default:
                return DistributedData::Asset::STATUS_NORMAL;
        }
    }
    return DistributedData::Asset::STATUS_UNKNOWN;
}

uint32_t ValueProxy::Asset::ConvertToDBStatus(const DistributedData::Asset &asset)
{
    switch (asset.status) {
        case DistributedData::Asset::STATUS_NORMAL:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::NORMAL);
        case DistributedData::Asset::STATUS_ABNORMAL:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::ABNORMAL);
        case DistributedData::Asset::STATUS_INSERT:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::INSERT);
        case DistributedData::Asset::STATUS_UPDATE:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::UPDATE);
        case DistributedData::Asset::STATUS_DELETE:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::DELETE);
        case DistributedData::Asset::STATUS_DOWNLOADING:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::DOWNLOADING);
        default:
            return static_cast<uint32_t>(DistributedDB::AssetStatus::NORMAL);
    }
}

ValueProxy::Assets::Assets(DistributedData::Assets assets)
{
    assets_.clear();
    assets_.reserve(assets.size());
    for (auto &asset : assets) {
        assets_.emplace_back(std::move(asset));
    }
}

ValueProxy::Assets::Assets(NativeRdb::ValueObject::Assets assets)
{
    assets_.clear();
    assets_.reserve(assets.size());
    for (auto &asset : assets) {
        assets_.emplace_back(std::move(asset));
    }
}

ValueProxy::Assets::Assets(DistributedDB::Assets assets)
{
    assets_.clear();
    assets_.reserve(assets.size());
    for (auto &asset : assets) {
        assets_.emplace_back(std::move(asset));
    }
}

ValueProxy::Assets &ValueProxy::Assets::operator=(const Assets &proxy)
{
    if (this == &proxy) {
        return *this;
    }
    assets_ = proxy.assets_;
    return *this;
}

ValueProxy::Assets &ValueProxy::Assets::operator=(Assets &&proxy) noexcept
{
    if (this == &proxy) {
        return *this;
    }
    assets_ = std::move(proxy.assets_);
    return *this;
}

ValueProxy::Assets::operator NativeRdb::ValueObject::Assets()
{
    NativeRdb::ValueObject::Assets assets;
    assets.reserve(assets_.size());
    for (auto &asset : assets_) {
        assets.push_back(std::move(asset));
    }
    return assets;
}

ValueProxy::Assets::operator DistributedData::Assets()
{
    DistributedData::Assets assets;
    assets.reserve(assets_.size());
    for (auto &asset : assets_) {
        assets.push_back(std::move(asset));
    }
    return assets;
}

ValueProxy::Assets::operator DistributedDB::Assets()
{
    DistributedDB::Assets assets;
    assets.reserve(assets_.size());
    for (auto &asset : assets_) {
        assets.push_back(std::move(asset));
    }
    return assets;
}

ValueProxy::Value &ValueProxy::Value::operator=(ValueProxy::Value &&value) noexcept
{
    if (this == &value) {
        return *this;
    }
    value_ = std::move(value.value_);
    return *this;
}

ValueProxy::Value::operator NativeRdb::ValueObject()
{
    NativeRdb::ValueObject object;
    DistributedData::Convert(std::move(value_), object.value);
    return object;
}

ValueProxy::Value::operator DistributedData::Value()
{
    DistributedData::Value value;
    DistributedData::Convert(std::move(value_), value);
    return value;
}

ValueProxy::Value::operator DistributedDB::Type()
{
    DistributedDB::Type value;
    DistributedData::Convert(std::move(value_), value);
    return value;
}

ValueProxy::Values &ValueProxy::Values::operator=(ValueProxy::Values &&values) noexcept
{
    if (this == &values) {
        return *this;
    }
    value_ = std::move(values.value_);
    return *this;
}

ValueProxy::Bucket &ValueProxy::Bucket::operator=(Bucket &&bucket) noexcept
{
    if (this == &bucket) {
        return *this;
    }
    value_ = std::move(bucket.value_);
    return *this;
}

ValueProxy::Bucket::operator NativeRdb::ValuesBucket()
{
    NativeRdb::ValuesBucket bucket;
    for (auto &[key, value] : value_) {
        bucket.values_.insert_or_assign(key, std::move(value));
    }
    value_.clear();
    return bucket;
}

ValueProxy::Buckets &ValueProxy::Buckets::operator=(Buckets &&buckets) noexcept
{
    if (this == &buckets) {
        return *this;
    }
    value_ = std::move(buckets.value_);
    return *this;
}
} // namespace OHOS::DistributedRdb
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
#define LOG_TAG "DeviceStoreImpl"
#include "device_store_impl.h"

#include <endian.h>
#include <regex>

#include "dev_manager.h"
#include "log_print.h"
namespace OHOS::DistributedKv {
std::vector<uint8_t> DeviceStoreImpl::ToLocalDBKey(const Key &key) const
{
    auto deviceId = DevManager::GetInstance().GetLocalDevice().deviceId;
    if (deviceId.empty()) {
        return {};
    }

    std::vector<uint8_t> input = SingleStoreImpl::GetPrefix(key);
    if (input.empty()) {
        return {};
    }

    // |local uuid|original key|uuid len|
    // |---- -----|------------|---4----|
    std::vector<uint8_t> dbKey;
    dbKey.insert(dbKey.end(), deviceId.begin(), deviceId.end());
    dbKey.insert(dbKey.end(), input.begin(), input.end());
    uint32_t length = deviceId.length();
    length = htole32(length);
    dbKey.insert(dbKey.end(), &length, &length + sizeof(length));
    return dbKey;
}

std::vector<uint8_t> DeviceStoreImpl::ToWholeDBKey(const Key &key) const
{
    // | device uuid | original key | uuid len |
    // |-------------|--------------|-----4----|
    return ConvertNetwork(key);
}

Key DeviceStoreImpl::ToKey(DBKey &&key) const
{
    // |  uuid    |original key|uuid len|
    // |---- -----|------------|---4----|
    uint32_t length = *(reinterpret_cast<uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
    length = le32toh(length);
    key.erase(key.begin(), key.begin() + length);
    key.erase(key.end() - sizeof(uint32_t), key.end());
    return std::move(key);
}

std::vector<uint8_t> DeviceStoreImpl::GetPrefix(const Key &prefix) const
{
    // |  uuid    |original key|
    // |---- -----|------------|
    return ConvertNetwork(prefix);
}

std::vector<uint8_t> DeviceStoreImpl::GetPrefix(const DataQuery &query) const
{
    std::vector<uint8_t> prefix;
    uint32_t length = query.deviceId_.size();
    prefix.insert(prefix.end(), &length, &length + sizeof(length));
    prefix.insert(prefix.end(), query.deviceId_.begin(), query.deviceId_.end());
    prefix.insert(prefix.end(), query.prefix_.begin(), query.prefix_.end());
    // |  uuid    |original key|
    // |---- -----|------------|
    return ConvertNetwork(std::move(prefix));
}

SingleStoreImpl::Convert DeviceStoreImpl::GetConvert() const
{
    return [](const DBKey &key, std::string &deviceId) {
        uint32_t length = *(reinterpret_cast<const uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
        length = le32toh(length);
        deviceId = { key.begin(), key.begin() + length };
        Key result(std::vector<uint8_t>(key.begin() + length, key.end() - sizeof(uint32_t)));
        return std::move(key);
    };
}

std::vector<uint8_t> DeviceStoreImpl::ConvertNetwork(const Key &in, bool withLen) const
{
    // input
    // | network ID len | networkID | original key|
    // |--------4-------|-----------|------------|
    if (in.Size() < sizeof(uint32_t)) {
        return in.Data();
    }
    size_t deviceLen = *(reinterpret_cast<const uint32_t *>(in.Data().data()));
    std::string deviceId(in.Data().begin() + sizeof(uint32_t), in.Data().begin() + sizeof(uint32_t) + deviceLen);
    std::regex patten("^[0-9]*$");
    if (!std::regex_match(deviceId, patten)) {
        ZLOGE("device id length is error.");
        return in.Data();
    }
    deviceId = DevManager::GetInstance().ToUUID(deviceId);
    if (deviceId.empty()) {
        return in.Data();
    }

    // output
    // | device uuid | original key | uuid len |
    // |-------------|--------------|----4-----|
    // |  Mandatory  |   Mandatory  | Optional |
    std::vector<uint8_t> out;
    out.insert(out.end(), deviceId.begin(), deviceId.end());
    out.insert(out.end(), in.Data().begin() + sizeof(uint32_t) + deviceLen, in.Data().end());
    if (withLen) {
        uint32_t length = deviceId.length();
        length = htole32(length);
        out.insert(out.end(), &length, &length + sizeof(length));
    }
    return out;
}
}
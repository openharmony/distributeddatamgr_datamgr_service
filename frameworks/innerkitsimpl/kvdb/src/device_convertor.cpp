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
#define LOG_TAG "DeviceConvertor"
#include "device_convertor.h"
#include <endian.h>
#include <iomanip>
#include <regex>
#include "dev_manager.h"
#include "log_print.h"
namespace OHOS::DistributedKv {
std::vector<uint8_t> DeviceConvertor::ToLocalDBKey(const Key &key) const
{
    return ToLocal(key, true);
}

std::vector<uint8_t> DeviceConvertor::ToLocal(const Key &in, bool withLen) const
{
    auto uuid = DevManager::GetInstance().GetLocalDevice().uuid;
    if (uuid.empty()) {
        return {};
    }

    std::vector<uint8_t> original = TrimKey(in);
    if ((original.empty() && withLen) || original.size() > MAX_DEV_KEY_LEN) {
        return {};
    }

    // |local uuid|original key|uuid len|
    // |---- -----|------------|---4----|
    std::vector<uint8_t> dbKey;
    dbKey.insert(dbKey.end(), uuid.begin(), uuid.end());
    dbKey.insert(dbKey.end(), original.begin(), original.end());
    if (withLen) {
        uint32_t length = uuid.length();
        length = htole32(length);
        uint8_t *buf = reinterpret_cast<uint8_t *>(&length);
        dbKey.insert(dbKey.end(), buf, buf + sizeof(length));
    }
    return dbKey;
}

std::vector<uint8_t> DeviceConvertor::ToWholeDBKey(const Key &key) const
{
    // | device uuid | original key | uuid len |
    // |-------------|--------------|-----4----|
    return ConvertNetwork(key, true);
}

Key DeviceConvertor::ToKey(DBKey &&key, std::string &deviceId) const
{
    // |  uuid    |original key|uuid len|
    // |---- -----|------------|---4----|
    if (key.size() < sizeof(uint32_t)) {
        return std::move(key);
    }

    uint32_t length = *(reinterpret_cast<uint32_t *>(&(*(key.end() - sizeof(uint32_t)))));
    if (length > key.size() - sizeof(uint32_t)) {
        return std::move(key);
    }

    if (deviceId.empty()) {
        deviceId = DevManager::GetInstance().ToNetworkId({ key.begin(), key.begin() + length });
    }

    length = le32toh(length);
    key.erase(key.begin(), key.begin() + length);
    key.erase(key.end() - sizeof(uint32_t), key.end());
    return std::move(key);
}

std::vector<uint8_t> DeviceConvertor::GetPrefix(const Key &prefix) const
{
    // |  length  | networkId | original key |
    // |----4-----|-----------|--------------|
    return ConvertNetwork(prefix);
}

std::vector<uint8_t> DeviceConvertor::GetPrefix(const DataQuery &query) const
{
    // |  length  | networkId | original key |
    // |----4-----|-----------|--------------|
    return ConvertNetwork(GetRealKey(query.prefix_, query));
}

std::string DeviceConvertor::GetRealKey(const std::string &key, const DataQuery &query) const
{
    if (query.deviceId_.empty()) {
        return key;
    }

    // |  length  | networkId | original key |
    // |----4-----|-----------|--------------|
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(sizeof(uint32_t)) << query.deviceId_.length();
    oss << query.deviceId_ << key;
    std::string realKey = oss.str();
    return realKey;
}

std::vector<uint8_t> DeviceConvertor::ConvertNetwork(const Key &in, bool withLen) const
{
    // input
    // | network ID len | networkID | original key |
    // |--------4-------|-----------|--------------|
    size_t begin = 0;
    if (in.Size() < sizeof(uint32_t)) {
        return ToLocal(in, withLen);
    }
    std::string deviceLen(in.Data().begin() + begin, in.Data().begin() + begin + sizeof(uint32_t));
    std::regex patten("^[0-9]*$");
    if (!std::regex_match(deviceLen, patten)) {
        // | original key |
        // |--------------|
        return ToLocal(in, withLen);
    }

    size_t devLen = static_cast<size_t>(atol(deviceLen.c_str()));
    if (devLen > in.Data().size() + sizeof(uint32_t)) {
        // | original key |
        // |--------------|
        return ToLocal(in, withLen);
    }
    begin += sizeof(uint32_t);

    std::string networkId(in.Data().begin() + begin, in.Data().begin() + begin + devLen);
    std::string uuid = DevManager::GetInstance().ToUUID(networkId);
    if (uuid.empty()) {
        // | original key |
        // |--------------|
        return devLen != DevManager::MAX_ID_LEN ? ToLocal(in, withLen) : std::vector<uint8_t>();
    }
    begin += devLen;

    // output
    // | device uuid | original key | uuid len |
    // |-------------|--------------|----4-----|
    // |  Mandatory  |   Mandatory  | Optional |
    std::vector<uint8_t> original{ in.Data().begin() + begin, in.Data().end() };
    original = TrimKey(std::move(original));
    if ((original.empty() && withLen) || original.size() > MAX_DEV_KEY_LEN) {
        return {};
    }

    std::vector<uint8_t> out;
    out.insert(out.end(), uuid.begin(), uuid.end());
    out.insert(out.end(), original.begin(), original.end());
    if (withLen) {
        uint32_t length = uuid.length();
        length = htole32(length);
        uint8_t *buf = reinterpret_cast<uint8_t *>(&length);
        out.insert(out.end(), buf, buf + sizeof(length));
    }
    return out;
}
} // namespace OHOS::DistributedKv
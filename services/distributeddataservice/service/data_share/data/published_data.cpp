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
#define LOG_TAG "PublishedData"
#include "published_data.h"

#include "log_print.h"
#include "subscriber_managers/published_data_subscriber_manager.h"
#include "base64_utils.h"

namespace OHOS::DataShare {
bool PublishedData::HasVersion() const
{
    return true;
}

int PublishedData::GetVersion() const
{
    return value.GetVersion();
}

std::string PublishedData::GetValue() const
{
    return DistributedData::Serializable::Marshall(value);
}

PublishedData::PublishedData(const PublishedDataNode &node, const int version) : PublishedData(node)
{
    value.SetVersion(version);
}

PublishedData::PublishedData(const PublishedDataNode &node)
    : KvData(Id(GenId(node.key, node.bundleName, node.subscriberId), node.userId)), value(node)
{
}

std::vector<PublishedData> PublishedData::Query(const std::string &bundleName, int32_t userId)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return std::vector<PublishedData>();
    }
    std::vector<std::string> queryResults;
    int32_t status = delegate->GetBatch(KvDBDelegate::DATA_TABLE,
        "{\"bundleName\":\"" + bundleName + "\", \"userId\": " + std::to_string(userId) + "}", "{}", queryResults);
    if (status != E_OK) {
        ZLOGE("db GetBatch failed, %{public}s %{public}d", bundleName.c_str(), status);
        return std::vector<PublishedData>();
    }
    std::vector<PublishedData> results;
    for (auto &result : queryResults) {
        PublishedDataNode data;
        if (PublishedDataNode::Unmarshall(result, data)) {
            results.emplace_back(data, userId);
        }
    }
    return results;
}

bool PublishedDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(key)], key);
    ret = ret && SetValue(node[GET_NAME(bundleName)], bundleName);
    ret = ret && SetValue(node[GET_NAME(subscriberId)], subscriberId);
    ret = ret && SetValue(node[GET_NAME(value)], value);
    ret = ret && SetValue(node[GET_NAME(timestamp)], timestamp);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    return ret && VersionData::Marshal(node);
}

bool PublishedDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(key), key);
    ret = ret && GetValue(node, GET_NAME(bundleName), bundleName);
    ret = ret && GetValue(node, GET_NAME(subscriberId), subscriberId);
    if (ret) {
        GetValue(node, GET_NAME(value), value);
        VersionData::Unmarshal(node);
    }
    ret = ret && GetValue(node, GET_NAME(timestamp), timestamp);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    return ret;
}

PublishedDataNode::PublishedDataNode(const std::string &key, const std::string &bundleName, int64_t subscriberId,
    const int32_t userId, const Data &value)
    : VersionData(-1), key(key), bundleName(bundleName), subscriberId(subscriberId), value(std::move(value)),
      userId(userId)
{
    auto now = time(nullptr);
    if (now > 0) {
        timestamp = now;
    }
}

PublishedDataNode::PublishedDataNode() : VersionData(-1) {}

std::variant<std::vector<uint8_t>, std::string> PublishedDataNode::MoveTo(const PublishedDataNode::Data &data)
{
    auto *valueStr = std::get_if<std::string>(&data);
    if (valueStr != nullptr) {
        return *valueStr;
    }
    auto *valueBytes = std::get_if<PublishedDataNode::BytesData>(&data);
    if (valueBytes != nullptr) {
        return Base64::Decode(valueBytes->data);
    }
    ZLOGE("error");
    return "";
}

PublishedDataNode::Data PublishedDataNode::MoveTo(std::variant<std::vector<uint8_t>, std::string> &data)
{
    auto *valueStr = std::get_if<std::string>(&data);
    if (valueStr != nullptr) {
        return *valueStr;
    }
    auto *valueBytes = std::get_if<std::vector<uint8_t>>(&data);
    if (valueBytes != nullptr) {
        std::string valueEncode = Base64::Encode(*valueBytes);
        return BytesData(std::move(valueEncode));
    }
    ZLOGE("error");
    return "";
}

int32_t PublishedData::Query(const std::string &filter, PublishedDataNode::Data &publishedData)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return E_ERROR;
    }
    std::string queryResult;
    int32_t status = delegate->Get(KvDBDelegate::DATA_TABLE, filter, "{}", queryResult);
    if (status != E_OK) {
        ZLOGE("db Get failed, %{public}s %{public}d", filter.c_str(), status);
        return status;
    }
    PublishedDataNode data;
    if (!PublishedDataNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return E_ERROR;
    }
    publishedData = std::move(data.value);
    return E_OK;
}

std::string PublishedData::GenId(const std::string &key, const std::string &bundleName, int64_t subscriberId)
{
    return key + "_" + std::to_string(subscriberId) + "_" + bundleName;
}

void PublishedData::Delete(const std::string &bundleName, const int32_t userId)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return;
    }
    int32_t status = delegate->Delete(KvDBDelegate::DATA_TABLE,
        "{\"bundleName\":\"" + bundleName + "\", \"userId\": " + std::to_string(userId) + "}");
    if (status != E_OK) {
        ZLOGE("db Delete failed, %{public}s %{public}d", bundleName.c_str(), status);
    }
}

void PublishedData::ClearAging()
{
    // published data is valid in 240 hours
    auto lastValidData =
        std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(240));
    auto lastValidTime = std::chrono::system_clock::to_time_t(lastValidData);
    if (lastValidTime <= 0) {
        return;
    }
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return;
    }
    std::vector<std::string> queryResults;
    int32_t status = delegate->GetBatch(KvDBDelegate::DATA_TABLE, "{}",
        "{\"id_\": true, \"timestamp\": true, \"key\": true, \"bundleName\": true, \"subscriberId\": true, "
        "\"userId\": true}",
        queryResults);
    if (status != E_OK) {
        ZLOGE("db GetBatch failed %{public}d", status);
        return;
    }
    int32_t agingSize = 0;
    for (auto &result : queryResults) {
        PublishedDataNode data;
        if (!PublishedDataNode::Unmarshall(result, data)) {
            ZLOGE("Unmarshall %{public}s failed", result.c_str());
            continue;
        }
        if (data.timestamp < lastValidTime && PublishedDataSubscriberManager::GetInstance()
            .GetCount(PublishedDataKey(data.key, data.bundleName, data.subscriberId)) == 0) {
            status = delegate->Delete(KvDBDelegate::DATA_TABLE,
                Id(PublishedData::GenId(data.key, data.bundleName, data.subscriberId), data.userId));
            if (status != E_OK) {
                ZLOGE("db Delete failed, %{public}s %{public}s", data.key.c_str(), data.bundleName.c_str());
            }
            agingSize++;
        }
    }
    if (agingSize > 0) {
        ZLOGI("aging count %{public}d", agingSize);
    }
    return;
}

void PublishedData::UpdateTimestamp(
    const std::string &key, const std::string &bundleName, int64_t subscriberId, const int32_t userId)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return;
    }
    std::string queryResult;
    int32_t status =
        delegate->Get(KvDBDelegate::DATA_TABLE, Id(GenId(key, bundleName, subscriberId), userId), queryResult);
    if (status != E_OK) {
        ZLOGE("db Get failed, %{private}s %{public}d", queryResult.c_str(), status);
        return;
    }
    PublishedDataNode data;
    if (!PublishedDataNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return;
    }
    auto now = time(nullptr);
    if (now <= 0) {
        ZLOGE("time failed");
        return;
    }
    data.timestamp = now;
    status = delegate->Upsert(KvDBDelegate::DATA_TABLE, PublishedData(data));
    if (status == E_OK) {
        ZLOGI("update timestamp %{private}s", data.key.c_str());
    }
}

PublishedDataNode::BytesData::BytesData(std::string &&data) : data(std::move(data))
{
}

bool PublishedDataNode::BytesData::Marshal(DistributedData::Serializable::json &node) const
{
    return SetValue(node[GET_NAME(data)], data);
}

bool PublishedDataNode::BytesData::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(data), data);
    return ret;
}
} // namespace OHOS::DataShare
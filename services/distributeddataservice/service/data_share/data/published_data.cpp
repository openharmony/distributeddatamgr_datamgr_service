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

PublishedData::PublishedData(const PublishedDataNode &node, const int version)
    : PublishedData(node)
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
    ret = ret && GetValue(node, GET_NAME(value), value);
    ret = ret && GetValue(node, GET_NAME(timestamp), timestamp);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    return ret && VersionData::Unmarshal(node);
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
    int32_t status = delegate->GetBatch(KvDBDelegate::DATA_TABLE, "{}", "{}", queryResults);
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
        if (data.timestamp < lastValidTime && PublishedDataSubscriberManager::GetInstance().GetCount(PublishedDataKey(
                                                  data.key, data.bundleName, data.subscriberId)) == 0) {
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
} // namespace OHOS::DataShare
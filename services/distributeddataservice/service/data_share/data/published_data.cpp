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

PublishedData::PublishedData(const std::string &key, const std::string &bundleName, int64_t subscriberId,
    const std::variant<std::vector<uint8_t>, std::string> &inputValue, const int version)
    : KvData(Id(GenId(key, bundleName, subscriberId))),
      value(key, bundleName, subscriberId, inputValue)
{
    value.SetVersion(version);
}
/*
std::vector<PublishedData> PublishedData::Query(const std::string &bundleName)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return std::vector<PublishedData>();
    }
    std::vector<std::string> queryResults;
    json filter;
    filter["bundleName"] = bundleName;
    int32_t status = delegate->GetBatch(KvDBDelegate::DATA_TABLE, filter.dump(), "{}", queryResults);
    if (status != E_OK) {
        ZLOGE("db Upsert failed, %{public}s %{public}d", bundleName.c_str(), status);
        return std::vector<PublishedData>();
    }
    std::vector<PublishedData> results;
    for (auto &result : queryResults) {
        PublishedData data;
        if (data.Unmarshall(result)) {
            results.push_back(std::move(data));
        }
    }
    return results;
} */

bool PublishedDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(key)], key);
    ret = ret && SetValue(node[GET_NAME(bundleName)], bundleName);
    ret = ret && SetValue(node[GET_NAME(subscriberId)], subscriberId);
    ret = ret && SetValue(node[GET_NAME(value)], value);
    ret = ret && SetValue(node[GET_NAME(timestamp)], timestamp);
    return ret && VersionData::Marshal(node);
}

bool PublishedDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(key), key);
    ret = ret && GetValue(node, GET_NAME(bundleName), bundleName);
    ret = ret && GetValue(node, GET_NAME(subscriberId), subscriberId);
    ret = ret && GetValue(node, GET_NAME(value), value);
    ret = ret && GetValue(node, GET_NAME(timestamp), timestamp);
    return ret && VersionData::Unmarshal(node);
}

PublishedDataNode::PublishedDataNode(const std::string &key, const std::string &bundleName,
    int64_t subscriberId, const std::variant<std::vector<uint8_t>, std::string> &value)
    : VersionData(-1), key(key), bundleName(bundleName), subscriberId(subscriberId), value(std::move(value))
{
    auto now = time(nullptr);
    if (now > 0) {
        timestamp = now;
    }
}

PublishedDataNode::PublishedDataNode() : VersionData(-1) {}

int32_t PublishedData::Query(const std::string &filter, std::variant<std::vector<uint8_t>, std::string> &publishedData)
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
} // namespace OHOS::DataShare
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

VersionData PublishedData::GetVersion() const
{
    return value.version;
}

const DistributedData::Serializable &PublishedData::GetValue() const
{
    return value;
}

bool PublishedData::Unmarshal(const DistributedData::Serializable::json &node)
{
    return value.Unmarshal(node);
}

PublishedData::PublishedData(const std::string &key, const std::string &bundleName, int64_t subscriberId,
    const std::variant<sptr<Ashmem>, std::string> &inputValue, const int version)
    : value(key, bundleName, subscriberId, inputValue, version)
{
}

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
}

bool PublishedDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node["key"], key);
    ret = ret && SetValue(node["bundleName"], bundleName);
    ret = ret && SetValue(node["subscriberId"], subscriberId);
    ret = ret && SetValue(node, version);
    if (value.index() == 1) {
        std::string valueStr = std::get<std::string>(value);
        ret = ret && SetValue(node["type"], PublishedData::STRING);
        ret = ret && SetValue(node["value"], valueStr);
    } else {
        sptr<Ashmem> ashmem = std::get<sptr<Ashmem>>(value);
        if (ashmem == nullptr) {
            ZLOGE("get ashmem null");
            return false;
        }
        const uint8_t *data = static_cast<const uint8_t *>(ashmem->ReadFromAshmem(ashmem->GetAshmemSize(), 0));
        if (data == nullptr) {
            ZLOGE("ReadFromAshmem null");
            return false;
        }
        ret = ret && SetValue(node["type"], PublishedData::ASHMEM);
        node["value"] = std::vector<uint8_t>(data, data + ashmem->GetAshmemSize());
    }
    std::time_t now = time(nullptr);
    if (now <= 0) {
        ZLOGE("time error");
        return false;
    }
    ret = ret && SetValue(node["timestamp"], now);
    return ret;
}

bool PublishedDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, "key", key);
    ret = ret && GetValue(node, "bundleName", bundleName);
    ret = ret && GetValue(node, "subscriberId", subscriberId);
    ret = ret && version.Unmarshal(node);
    int32_t type = 0;
    ret = ret && GetValue(node, "type", type);
    if (!ret) {
        ZLOGE("Unmarshal PublishedDataNode failed, %{private}s", key.c_str());
        return false;
    }
    if (type == PublishedData::STRING) {
        std::string strValue;
        ret = ret && GetValue(node, "value", strValue);
        value = strValue;
    } else if (type == PublishedData::ASHMEM) {
        std::vector<uint8_t> binaryData = node["value"];
        std::string ashmemName = "PublishedData" + key + "_" + bundleName + "_" + std::to_string(subscriberId);
        auto ashmem = Ashmem::CreateAshmem(ashmemName.c_str(), binaryData.size());
        if (ashmem == nullptr) {
            ZLOGE("SharedBlock: CreateAshmem function error.");
            return false;
        }

        ret = ashmem->MapReadAndWriteAshmem();
        if (!ret) {
            ZLOGE("SharedBlock: MapReadAndWriteAshmem function error.");
            ashmem->CloseAshmem();
            return false;
        }
        ashmem->WriteToAshmem(&binaryData[0], binaryData.size(), 0);
        value = ashmem;
    }
    return ret;
}

PublishedDataNode::PublishedDataNode(const std::string &key, const std::string &bundleName, int64_t subscriberId,
    const std::variant<sptr<Ashmem>, std::string> &value, const int version)
    : key(key), bundleName(bundleName), subscriberId(subscriberId), value(value), version(version)
{
}

int32_t PublishedData::Query(const std::string &filter, std::variant<sptr<Ashmem>, std::string> &publishedData)
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
    PublishedData data;
    if (!data.Unmarshall(queryResult)) {
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return E_ERROR;
    }
    publishedData = data.value.value;
    return E_OK;
}
} // namespace OHOS::DataShare
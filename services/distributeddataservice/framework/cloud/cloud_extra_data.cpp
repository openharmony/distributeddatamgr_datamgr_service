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

#include "cloud/cloud_extra_data.h"
#include "cloud/cloud_config_manager.h"

namespace OHOS::DistributedData {
bool ExtensionInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(accountId)], accountId);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(containerName)], containerName);
    SetValue(node[GET_NAME(databaseScopes)], databaseScopes);
    SetValue(node[GET_NAME(recordTypes)], recordTypes);
    return true;
}

bool ExtensionInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(accountId), accountId);
    GetValue(node, GET_NAME(bundleName), bundleName);
    bundleName = CloudConfigManager::GetInstance().ToLocal(bundleName);
    GetValue(node, GET_NAME(containerName), containerName);
    GetValue(node, GET_NAME(databaseScopes), databaseScopes);
    if (!Unmarshall(databaseScopes, scopes)) {
        return false;
    }
    GetValue(node, GET_NAME(recordTypes), recordTypes);
    return Unmarshall(recordTypes, tables);
}

bool ExtraData::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(header)], header);
    SetValue(node[GET_NAME(data)], data);
    return true;
}

bool ExtraData::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(header), header);
    GetValue(node, GET_NAME(data), data);
    return info.Unmarshall(data);
}

bool ExtraData::isPrivate() const
{
    return (std::find(info.scopes.begin(), info.scopes.end(), PRIVATE_TABLE) != info.scopes.end());
}

bool ExtraData::isShared() const
{
    return (std::find(info.scopes.begin(), info.scopes.end(), SHARED_TABLE) != info.scopes.end());
}
} // namespace OHOS::DistributedData
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

#include "cloud/cloud_info.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
bool CloudInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(user)], user);
    SetValue(node[GET_NAME(id)], id);
    SetValue(node[GET_NAME(totalSpace)], totalSpace);
    SetValue(node[GET_NAME(remainSpace)], remainSpace);
    SetValue(node[GET_NAME(enableCloud)], enableCloud);
    SetValue(node[GET_NAME(apps)], apps);
    return true;
}

bool CloudInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(user), user);
    GetValue(node, GET_NAME(id), id);
    GetValue(node, GET_NAME(totalSpace), totalSpace);
    GetValue(node, GET_NAME(remainSpace), remainSpace);
    GetValue(node, GET_NAME(enableCloud), enableCloud);
    GetValue(node, GET_NAME(apps), apps);
    return true;
}

bool CloudInfo::AppInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(instanceId)], instanceId);
    SetValue(node[GET_NAME(cloudSwitch)], cloudSwitch);
    return true;
}

bool CloudInfo::AppInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(instanceId), instanceId);
    GetValue(node, GET_NAME(cloudSwitch), cloudSwitch);
    return true;
}

std::string CloudInfo::GetKey() const
{
    return GetKey(INFO_PREFIX, { std::to_string(user) });
}

std::map<std::string, std::string> CloudInfo::GetSchemaKey() const
{
    std::map<std::string, std::string> keys;
    for (const auto &[bundle, app] : apps) {
        const auto key = GetKey(
            SCHEMA_PREFIX, { std::to_string(user), bundle, std::to_string(app.instanceId) });
        keys.insert_or_assign(app.bundleName, key);
    }
    return keys;
}

std::string CloudInfo::GetSchemaKey(const std::string &bundleName, int32_t instanceId) const
{
    return GetKey(SCHEMA_PREFIX, { std::to_string(user), bundleName, std::to_string(instanceId) });
}

std::string CloudInfo::GetSchemaKey(int32_t user, const std::string &bundleName, int32_t instanceId)
{
    return GetKey(SCHEMA_PREFIX, { std::to_string(user), bundleName, std::to_string(instanceId) });
}

std::string CloudInfo::GetSchemaPrefix(const std::string &bundleName) const
{
    if (bundleName.empty()) {
        return GetKey(SCHEMA_PREFIX, { std::to_string(user) });
    }
    return GetKey(SCHEMA_PREFIX, { std::to_string(user), bundleName});
}

std::string CloudInfo::GetSchemaKey(const StoreMetaData &meta)
{
    return GetKey(SCHEMA_PREFIX, { meta.user,  meta.bundleName, std::to_string(meta.instanceId) });
}

bool CloudInfo::IsValid() const
{
    return !id.empty();
}

bool CloudInfo::Exist(const std::string &bundleName, int32_t instanceId)
{
    if (bundleName.empty()) {
        return false;
    }
    auto it = apps.find(bundleName);
    return it != apps.end() && it->second.bundleName == bundleName && it->second.instanceId == instanceId;
}

bool CloudInfo::IsOn(const std::string &bundleName, int32_t instanceId)
{
    if (bundleName.empty()) {
        return false;
    }
    auto it = apps.find(bundleName);
    return it != apps.end() && it->second.instanceId == instanceId && it->second.cloudSwitch;
}

bool CloudInfo::IsAllSwitchOff() const
{
    for (auto &[bundle, app] : apps) {
        if (app.cloudSwitch) {
            return false;
        }
    }
    return true;
}

std::string CloudInfo::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return GetKey(INFO_PREFIX, fields).append(Constant::KEY_SEPARATOR);
}

std::string CloudInfo::GetKey(const std::string &prefix, const std::initializer_list<std::string> &fields)
{
    return Constant::Join(prefix, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData
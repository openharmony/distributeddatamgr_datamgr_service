/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "CloudLastSyncInfo"
#include "cloud/cloud_last_sync_info.h"

#include "utils/constant.h"
namespace OHOS::DistributedData {
bool CloudLastSyncInfo::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(id)], id);
    SetValue(node[GET_NAME(storeId)], storeId);
    SetValue(node[GET_NAME(startTime)], startTime);
    SetValue(node[GET_NAME(finishTime)], finishTime);
    SetValue(node[GET_NAME(code)], code);
    SetValue(node[GET_NAME(syncStatus)], syncStatus);
    SetValue(node[GET_NAME(instanceId)], instanceId);
    return true;
}

bool CloudLastSyncInfo::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(id), id);
    GetValue(node, GET_NAME(storeId), storeId);
    GetValue(node, GET_NAME(startTime), startTime);
    GetValue(node, GET_NAME(finishTime), finishTime);
    GetValue(node, GET_NAME(code), code);
    GetValue(node, GET_NAME(syncStatus), syncStatus);
    GetValue(node, GET_NAME(instanceId), instanceId);
    return true;
}

std::string CloudLastSyncInfo::GetKey(const int32_t user, const std::string &bundleName,
                                      const std::string &storeId, int32_t instanceId)
{
    return GetKey(INFO_PREFIX, { std::to_string(user), bundleName, std::to_string(instanceId), storeId });
}

std::string CloudLastSyncInfo::GetKey(const std::string &prefix, const std::initializer_list<std::string> &fields)
{
    return Constant::Join(prefix, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData
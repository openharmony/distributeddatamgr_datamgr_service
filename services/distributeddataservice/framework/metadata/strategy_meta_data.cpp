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
#include "metadata/strategy_meta_data.h"

#include "utils/constant.h"
namespace OHOS::DistributedData {
bool StrategyMeta::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(devId)], devId) && ret;
    ret = SetValue(node[GET_NAME(userId)], userId) && ret;
    ret = SetValue(node[GET_NAME(bundleName)], bundleName) && ret;
    ret = SetValue(node[GET_NAME(instanceId)], instanceId) && ret;
    ret = SetValue(node[GET_NAME(storeId)], storeId) && ret;
    ret = SetValue(node[GET_NAME(capabilityEnabled)], capabilityEnabled) && ret;
    ret = SetValue(node[GET_NAME(capabilityRange)], capabilityRange) && ret;
    return ret;
}

bool StrategyMeta::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(devId), devId) && ret;
    ret = GetValue(node, GET_NAME(userId), userId) && ret;
    ret = GetValue(node, GET_NAME(bundleName), bundleName) && ret;
    ret = GetValue(node, GET_NAME(instanceId), instanceId) && ret;
    ret = GetValue(node, GET_NAME(storeId), storeId) && ret;
    ret = GetValue(node, GET_NAME(capabilityEnabled), capabilityEnabled) && ret;
    ret = GetValue(node, GET_NAME(capabilityRange), capabilityRange) && ret;
    return ret;
}

StrategyMeta::StrategyMeta(
    const std::string &devId, const std::string &userId, const std::string &bundleName, const std::string &storeId)
    : devId(devId), userId(userId), bundleName(bundleName), storeId(storeId)
{
}

bool StrategyMeta::IsEffect() const
{
    return (!capabilityRange.localLabel.empty()) && (!capabilityRange.remoteLabel.empty());
}

std::string StrategyMeta::GetKey()
{
    if (instanceId == 0) {
        return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, { devId, userId, "default", bundleName, storeId });
    }
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR,
        { devId, userId, "default", bundleName, storeId, std::to_string(instanceId) });
}

std::string StrategyMeta::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData

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
namespace OHOS::DistributedData {
bool StrategyMeta::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(devId)], devId) && ret;
    ret = SetValue(node[GET_NAME(devAccId)], devAccId) && ret;
    ret = SetValue(node[GET_NAME(grpId)], grpId) && ret;
    ret = SetValue(node[GET_NAME(bundleName)], bundleName) && ret;
    ret = SetValue(node[GET_NAME(storeId)], storeId) && ret;
    return ret;
}

bool StrategyMeta::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(devId), devId) && ret;
    ret = GetValue(node, GET_NAME(devAccId), devAccId) && ret;
    ret = GetValue(node, GET_NAME(grpId), grpId) && ret;
    ret = GetValue(node, GET_NAME(bundleName), bundleName) && ret;
    ret = GetValue(node, GET_NAME(storeId), storeId) && ret;
    return ret;
}
StrategyMeta::StrategyMeta(const std::string &devId, const std::string &devAccId, const std::string &grpId,
    const std::string &bundleName, const std::string &storeId)
    : devId(devId), devAccId(devAccId), grpId(grpId), bundleName(bundleName), storeId(storeId)
{
}
} // namespace OHOS::DistributedData

/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "model/checker_config.h"
namespace OHOS {
namespace DistributedData {
bool CheckerConfig::Trust::Marshal(json &node) const
{
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(packageName)], packageName);
    SetValue(node[GET_NAME(base64Key)], base64Key);
    SetValue(node[GET_NAME(checker)], checker);
    return true;
}

bool CheckerConfig::Trust::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(packageName), packageName);
    GetValue(node, GET_NAME(base64Key), base64Key);
    GetValue(node, GET_NAME(checker), checker);
    return true;
}

bool CheckerConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(checkers)], checkers);
    SetValue(node[GET_NAME(trusts)], trusts);
    return true;
}

bool CheckerConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(checkers), checkers);
    GetValue(node, GET_NAME(trusts), trusts);
    return true;
}
} // namespace DistributedData
} // namespace OHOS

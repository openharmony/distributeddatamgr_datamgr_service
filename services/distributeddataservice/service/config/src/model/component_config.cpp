/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "model/component_config.h"
namespace OHOS {
namespace DistributedData {
bool ComponentConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(description)], description);
    SetValue(node[GET_NAME(lib)], lib);
    SetValue(node[GET_NAME(constructor)], constructor);
    SetValue(node[GET_NAME(destructor)], destructor);
    if (!params.empty()) {
        node[GET_NAME(params)] = ToJson(params);
    }
    return true;
}

bool ComponentConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(description), description);
    GetValue(node, GET_NAME(lib), lib);
    GetValue(node, GET_NAME(constructor), constructor);
    GetValue(node, GET_NAME(destructor), destructor);
    const auto &subNode = GetSubNode(node, GET_NAME(params));
    if (!subNode.is_null()) {
        params = to_string(subNode);
    }
    return true;
}
} // namespace DistributedData
} // namespace OHOS

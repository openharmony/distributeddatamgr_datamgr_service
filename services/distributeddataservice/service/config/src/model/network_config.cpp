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

#include "model/network_config.h"
namespace OHOS {
namespace DistributedData {
bool NetworkConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(chains)], chains);
    SetValue(node[GET_NAME(routers)], routers);
    SetValue(node[GET_NAME(transports)], transports);
    SetValue(node[GET_NAME(protocols)], protocols);
    return true;
}

bool NetworkConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(chains), chains);
    GetValue(node, GET_NAME(routers), routers);
    GetValue(node, GET_NAME(transports), transports);
    GetValue(node, GET_NAME(protocols), protocols);
    return true;
}
} // namespace DistributedData
} // namespace OHOS

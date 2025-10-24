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
#include "model/double_sync_config.h"
#include "sync_mgr/sync_mgr.h"

namespace OHOS {
namespace DistributedData {
bool DoubleSyncConfig::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    return true;
}

bool DoubleSyncConfig::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(bundleName), bundleName);
    return true;
}
} // namespace DistributedData
} // namespace OHOS
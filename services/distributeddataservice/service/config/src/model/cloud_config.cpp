/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "model/cloud_config.h"
namespace OHOS {
namespace DistributedData {
bool CloudConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(mapper)], mapper);
    return true;
}

bool CloudConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(mapper), mapper);
    return true;
}

bool CloudConfig::Info::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(localBundle)], localBundle);
    SetValue(node[GET_NAME(cloudBundle)], cloudBundle);
    return true;
}
bool CloudConfig::Info::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(localBundle), localBundle);
    GetValue(node, GET_NAME(cloudBundle), cloudBundle);
    return true;
}
} // namespace DistributedData
} // namespace OHOS

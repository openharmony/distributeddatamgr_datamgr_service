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
#include "metadata/deviceid_meta_data.h"

namespace OHOS {
namespace DistributedData {
bool DeviceIDMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(currentUUID)], currentUUID);
    SetValue(node[GET_NAME(oldUUID)], oldUUID);
    return true;
}

bool DeviceIDMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(currentUUID), currentUUID);
    GetValue(node, GET_NAME(oldUUID), oldUUID);
    return true;
}

DeviceIDMetaData::DeviceIDMetaData()
{
}

DeviceIDMetaData::~DeviceIDMetaData()
{
}

std::string DeviceIDMetaData::GetKey() const
{
    return KEY_PREFIX;
}
} // namespace DistributedData
} // namespace OHOS
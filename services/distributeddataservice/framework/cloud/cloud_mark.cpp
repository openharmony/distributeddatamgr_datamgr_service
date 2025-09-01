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

#include "cloud/cloud_mark.h"

namespace OHOS::DistributedData {

static constexpr const char *KEY_PREFIX = "CloudMark";
static constexpr const char *KEY_SEPARATOR = "###";
bool CloudMark::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(isClearWaterMark)], isClearWaterMark);
    return true;
}

bool CloudMark::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(isClearWaterMark), isClearWaterMark);
    return true;
}

bool CloudMark::operator==(const CloudMark &cloudMark) const
{
    return (bundleName == cloudMark.bundleName) && (deviceId == cloudMark.deviceId) && (index == cloudMark.index) &&
        (isClearWaterMark == cloudMark.isClearWaterMark) && (storeId == cloudMark.storeId) &&
        (userId == cloudMark.userId);
}

bool CloudMark::operator!=(const CloudMark &cloudMark) const
{
    return !operator==(cloudMark);
}

std::string CloudMark::GetKey()
{
    return GetKey({ deviceId, std::to_string(userId), "default", bundleName, storeId, std::to_string(index) });
}

std::string CloudMark::GetKey(const std::initializer_list<std::string> &fields)
{
    std::string prefix = KEY_PREFIX;
    for (const auto &field : fields) {
        prefix.append(KEY_SEPARATOR).append(field);
    }
    return prefix;
}
} // namespace OHOS::DistributedData
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

#include "metadata/special_channel_data.h"
#include "utils/constant.h"
namespace OHOS::DistributedData {
bool SpecialChannelData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(devices)], devices);
    return true;
}

bool SpecialChannelData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(devices), devices);
    return true;
}

std::string SpecialChannelData::GetKey()
{
    return std::string(KEY_PREFIX) + Constant::KEY_SEPARATOR;
}
}
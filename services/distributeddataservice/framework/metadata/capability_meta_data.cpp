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

#include "metadata/capability_meta_data.h"

#include "utils/constant.h"
namespace OHOS::DistributedData {
constexpr int32_t CapMetaData::CURRENT_VERSION;
constexpr int32_t CapMetaData::INVALID_VERSION;
constexpr const char *CapMetaRow::KEY_PREFIX;
bool CapMetaData::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(version)], version) && ret;
    return ret;
}

bool CapMetaData::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(version), version) && ret;
    return ret;
}

std::vector<uint8_t> CapMetaRow::GetKeyFor(const std::string &key)
{
    std::string str = Constant::Concatenate({ KEY_PREFIX, Constant::KEY_SEPARATOR, key });
    return { str.begin(), str.end() };
}
} // namespace OHOS::DistributedData

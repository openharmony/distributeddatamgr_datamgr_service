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

#include "metadata/version_meta_data.h"

namespace OHOS::DistributedData {
VersionMetaData::VersionMetaData()
{
}

VersionMetaData::~VersionMetaData()
{
}

bool VersionMetaData::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(version)], version) && ret;
    return ret;
}

bool VersionMetaData::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(version), version) && ret;
    return ret;
}

std::string VersionMetaData::GetKey() const
{
    return KEY_PREFIX;
}
} // namespace OHOS::DistributedData
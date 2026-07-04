/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "metadata/bundle_version_meta_data.h"

namespace OHOS::DistributedData {
BundleVersionMetaData::BundleVersionMetaData()
{
}

BundleVersionMetaData::~BundleVersionMetaData()
{
}

bool BundleVersionMetaData::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(bundleName)], bundleName) && ret;
    ret = SetValue(node[GET_NAME(user)], user) && ret;
    ret = SetValue(node[GET_NAME(appIndex)], appIndex) && ret;
    ret = SetValue(node[GET_NAME(versionCode)], versionCode) && ret;
    return ret;
}

bool BundleVersionMetaData::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(bundleName), bundleName) && ret;
    ret = GetValue(node, GET_NAME(user), user) && ret;
    ret = GetValue(node, GET_NAME(appIndex), appIndex) && ret;
    ret = GetValue(node, GET_NAME(versionCode), versionCode) && ret;
    return ret;
}

std::string BundleVersionMetaData::GetKey() const
{
    return GetKey({user, bundleName, std::to_string(appIndex)});
}

std::string BundleVersionMetaData::GetKey(const std::initializer_list<std::string> &fields)
{
    std::string prefix = KEY_PREFIX;
    for (const auto &field : fields) {
        prefix.append(Constant::KEY_SEPARATOR).append(field);
    }
    return prefix;
}

std::string BundleVersionMetaData::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return GetKey(fields).append(Constant::KEY_SEPARATOR);
}
} // namespace OHOS::DistributedData

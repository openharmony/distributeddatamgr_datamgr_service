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
#include "metadata/secret_key_meta_data.h"

#include "utils/constant.h"
namespace OHOS {
namespace DistributedData {
SecretKeyMetaData::SecretKeyMetaData()
{
}

SecretKeyMetaData::~SecretKeyMetaData()
{
    sKey.assign(sKey.size(), 0);
}

bool SecretKeyMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(time)], time);
    SetValue(node[GET_NAME(sKey)], sKey);
    SetValue(node[GET_NAME(storeType)], storeType);
    return true;
}

bool SecretKeyMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(time), time);
    GetValue(node, GET_NAME(sKey), sKey);
    GetValue(node, GET_NAME(storeType), storeType);
    return true;
}

std::string SecretKeyMetaData::GetKey(const std::initializer_list<std::string> &fields)
{
    std::string prefix = GetPrefix(fields);
    prefix.append("SINGLE_KEY");
    return prefix;
}

std::string SecretKeyMetaData::GetBackupKey(const std::initializer_list<std::string> &fields)
{
    std::string prefix = GetBackupPrefix(fields);
    return prefix;
}

std::string SecretKeyMetaData::GetPrefix(const std::initializer_list<std::string> &fields)
{
    std::string prefix = KEY_PREFIX;
    for (const auto &field : fields) {
        prefix.append(Constant::KEY_SEPARATOR).append(field);
    }
    prefix.append(Constant::KEY_SEPARATOR);
    return prefix;
}

std::string SecretKeyMetaData::GetBackupPrefix(const std::initializer_list<std::string> &fields)
{
    std::string prefix = BACKUP_KEY_PREFIX;
    for (const auto &field : fields) {
        prefix.append(Constant::KEY_SEPARATOR).append(field);
    }
    prefix.append(Constant::KEY_SEPARATOR);
    return prefix;
}
} // namespace DistributedData
} // namespace OHOS

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
#include "metadata/store_meta_data_local.h"
#include "utils/constant.h"

namespace OHOS {
namespace DistributedData {
bool PolicyValue::Marshal(json &node) const
{
    SetValue(node[GET_NAME(type)], type);
    SetValue(node[GET_NAME(valueUint)], valueUint);
    SetValue(node[GET_NAME(index)], index);
    return true;
}

bool PolicyValue::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(type), type);
    GetValue(node, GET_NAME(valueUint), valueUint);
    GetValue(node, GET_NAME(index), index);
    return true;
}

bool PolicyValue::IsValueEffect() const
{
    return (index > 0);
}

bool StoreMetaDataLocal::HasPolicy(uint32_t type)
{
    for (auto &policy : policies) {
        if (policy.type == type) {
            return true;
        }
    }
    return false;
}

PolicyValue StoreMetaDataLocal::GetPolicy(uint32_t type)
{
    for (auto &policy : policies) {
        if (policy.type == type) {
            return policy;
        }
    }
    return PolicyValue();
}

bool StoreMetaDataLocal::Marshal(json &node) const
{
    SetValue(node[GET_NAME(isAutoSync)], isAutoSync);
    SetValue(node[GET_NAME(isBackup)], isBackup);
    SetValue(node[GET_NAME(isDirty)], isDirty);
    SetValue(node[GET_NAME(isEncrypt)], isEncrypt);
    SetValue(node[GET_NAME(dataDir)], dataDir);
    SetValue(node[GET_NAME(schema)], schema);
    SetValue(node[GET_NAME(policies)], policies);
    return true;
}

bool StoreMetaDataLocal::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(isAutoSync), isAutoSync);
    GetValue(node, GET_NAME(isBackup), isBackup);
    GetValue(node, GET_NAME(isDirty), isDirty);
    GetValue(node, GET_NAME(isEncrypt), isEncrypt);
    GetValue(node, GET_NAME(dataDir), dataDir);
    GetValue(node, GET_NAME(schema), schema);
    GetValue(node, GET_NAME(policies), policies);
    return true;
}

StoreMetaDataLocal::StoreMetaDataLocal()
{
}

StoreMetaDataLocal::~StoreMetaDataLocal()
{
}

bool StoreMetaDataLocal::operator==(const StoreMetaDataLocal &metaData) const
{
    if (Constant::NotEqual(isAutoSync, metaData.isAutoSync) || Constant::NotEqual(isBackup, metaData.isBackup) ||
        Constant::NotEqual(isDirty, metaData.isDirty) || Constant::NotEqual(isEncrypt, metaData.isEncrypt)) {
        return false;
    }
    return dataDir == metaData.dataDir;
}

bool StoreMetaDataLocal::operator!=(const StoreMetaDataLocal &metaData) const
{
    return !(*this == metaData);
}

std::string StoreMetaDataLocal::GetKey(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, fields);
}

std::string StoreMetaDataLocal::GetPrefix(const std::initializer_list<std::string> &fields)
{
    auto prefix = Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, fields);
    prefix.append(Constant::KEY_SEPARATOR);
    return prefix;
}
} // namespace DistributedData
} // namespace OHOS

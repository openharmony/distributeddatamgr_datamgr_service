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

#include "metadata/store_meta_data.h"

#include "utils/constant.h"
namespace OHOS {
namespace DistributedData {
using namespace OHOS::DistributedKv;
bool StoreMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(appType)], appType);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(dataDir)], dataDir);
    SetValue(node[GET_NAME(deviceAccountID)], deviceAccountId);
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(isAutoSync)], isAutoSync);
    SetValue(node[GET_NAME(isBackup)], isBackup);
    SetValue(node[GET_NAME(isEncrypt)], isEncrypt);
    SetValue(node[GET_NAME(kvStoreType)], kvStoreType);
    SetValue(node[GET_NAME(schema)], schema);
    SetValue(node[GET_NAME(storeId)], storeId);
    SetValue(node[GET_NAME(UID)], uid);
    SetValue(node[GET_NAME(userId)], userId);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(securityLevel)], securityLevel);
    SetValue(node[GET_NAME(isDirty)], isDirty);
    return true;
}
bool StoreMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(appType), appType);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(dataDir), dataDir);
    GetValue(node, GET_NAME(deviceAccountID), deviceAccountId);
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(isAutoSync), isAutoSync);
    GetValue(node, GET_NAME(isBackup), isBackup);
    GetValue(node, GET_NAME(isEncrypt), isEncrypt);
    GetValue(node, GET_NAME(kvStoreType), kvStoreType);
    GetValue(node, GET_NAME(schema), schema);
    GetValue(node, GET_NAME(storeId), storeId);
    GetValue(node, GET_NAME(UID), uid);
    GetValue(node, GET_NAME(userId), userId);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(securityLevel), securityLevel);
    GetValue(node, GET_NAME(isDirty), isDirty);
    return true;
}
StoreMetaData::StoreMetaData()
{
}
StoreMetaData::~StoreMetaData()
{
}
StoreMetaData::StoreMetaData(const std::string &appId, const std::string &storeId, const std::string &userId)
    : appId(appId), storeId(storeId), userId(userId)
{
}

// the Key Prefix for Meta data of KvStore.
const std::string KvStoreMetaRow::KEY_PREFIX = "KvStoreMetaData";
std::vector<uint8_t> KvStoreMetaRow::GetKeyFor(const std::string &key)
{
    std::string str = Constant::Concatenate({ KvStoreMetaRow::KEY_PREFIX, Constant::KEY_SEPARATOR, key });
    return { str.begin(), str.end() };
}

const std::string SecretMetaRow::KEY_PREFIX = "SecretKey";
std::vector<uint8_t> SecretMetaRow::GetKeyFor(const std::string &key)
{
    std::string str = Constant::Concatenate({ SecretMetaRow::KEY_PREFIX, Constant::KEY_SEPARATOR, key });
    return { str.begin(), str.end() };
}
} // namespace DistributedData
} // namespace OHOS

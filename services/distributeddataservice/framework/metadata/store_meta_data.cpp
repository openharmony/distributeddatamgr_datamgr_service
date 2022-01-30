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
namespace OHOS {
namespace DistributedData {
bool StoreMetaData::Marshal(Serializable::json &node) const
{
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(appType)], appType);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(dataDir)], dataDir);
    SetValue(node[GET_NAME(deviceAccountId)], deviceAccountId);
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(isAutoSync)], isAutoSync);
    SetValue(node[GET_NAME(isBackup)], isBackup);
    SetValue(node[GET_NAME(isEncrypt)], isEncrypt);
    SetValue(node[GET_NAME(kvStoreType)], kvStoreType);
    SetValue(node[GET_NAME(schema)], schema);
    SetValue(node[GET_NAME(storeId)], storeId);
    SetValue(node[GET_NAME(uid)], uid);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(securityLevel)], securityLevel);
    SetValue(node[GET_NAME(isDirty)], isDirty);
    return true;
}
bool StoreMetaData::Unmarshal(const Serializable::json &node)
{
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(appType), appType);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(dataDir), dataDir);
    GetValue(node, GET_NAME(deviceAccountId), deviceAccountId);
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(isAutoSync), isAutoSync);
    GetValue(node, GET_NAME(isBackup), isBackup);
    GetValue(node, GET_NAME(isEncrypt), isEncrypt);
    GetValue(node, GET_NAME(kvStoreType), kvStoreType);
    GetValue(node, GET_NAME(schema), schema);
    GetValue(node, GET_NAME(storeId), storeId);
    GetValue(node, GET_NAME(uid), uid);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(securityLevel), securityLevel);
    GetValue(node, GET_NAME(isDirty), isDirty);
    return true;
}
}
}
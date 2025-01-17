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
#include "secret_key_backup_data.h"
namespace OHOS {
namespace DistributedData {
SecretKeyBackupData::SecretKeyBackupData()
{
}

SecretKeyBackupData::~SecretKeyBackupData()
{
}

bool SecretKeyBackupData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(infos)], infos);
    return true;
}

bool SecretKeyBackupData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(infos), infos);
    return true;
}

SecretKeyBackupData::BackupItem::BackupItem()
{
}

SecretKeyBackupData::BackupItem::~BackupItem()
{
    sKey.assign(sKey.size(), 0);
}

bool SecretKeyBackupData::BackupItem::Marshal(json &node) const
{
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(dbName)], dbName);
    SetValue(node[GET_NAME(instanceId)], instanceId);
    SetValue(node[GET_NAME(user)], user);
    SetValue(node[GET_NAME(time)], time);
    SetValue(node[GET_NAME(sKey)], sKey);
    SetValue(node[GET_NAME(storeType)], storeType);
    return true;
}

bool SecretKeyBackupData::BackupItem::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(dbName), dbName);
    GetValue(node, GET_NAME(instanceId), instanceId);
    GetValue(node, GET_NAME(user), user);
    GetValue(node, GET_NAME(time), time);
    GetValue(node, GET_NAME(sKey), sKey);
    GetValue(node, GET_NAME(storeType), storeType);
    return true;
}

bool SecretKeyBackupData::BackupItem::IsValid() const
{
    return !bundleName.empty() && !dbName.empty() && !time.empty() && !sKey.empty() && !user.empty();
}
} // namespace DistributedData
} // namespace OHOS

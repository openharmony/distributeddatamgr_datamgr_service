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
#define LOG_TAG "PermissionProxy"
#include "permission_proxy.h"

#include "account/account_delegate.h"
#include "communication_provider.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"

namespace OHOS::DataShare {
bool PermissionProxy::QueryWritePermission(const std::string &bundleName, const std::string &moduleName,
    const std::string &storeName, std::string &permission)
{
    DistributedData::StoreMetaData meta;
    bool existed = QueryMetaData(bundleName, moduleName, storeName, meta);
    if (!existed) {
        ZLOGE("QueryMetaData fail");
        return false;
    }
    permission = meta.writePermission;
    return true;
}

bool PermissionProxy::QueryReadPermission(const std::string &bundleName, const std::string &moduleName,
    const std::string &storeName, std::string &permission)
{
    DistributedData::StoreMetaData meta;
    bool existed = QueryMetaData(bundleName, moduleName, storeName, meta);
    if (!existed) {
        ZLOGE("QueryMetaData fail");
        return false;
    }
    permission = meta.writePermission;
    return true;
}

void PermissionProxy::FillData(DistributedData::StoreMetaData &meta)
{
    meta.deviceId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    meta.user = DistributedKv::AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(IPCSkeleton::GetCallingUid());
}

bool PermissionProxy::QueryMetaData(const std::string &bundleName, const std::string &moduleName,
    const std::string &storeName, DistributedData::StoreMetaData &metaData)
{
    DistributedData::StoreMetaData meta;
    FillData(meta);
    meta.bundleName = bundleName;
    meta.storeId = storeName;

    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData);
    if (!isCreated) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}
} // namespace OHOS::DataShare
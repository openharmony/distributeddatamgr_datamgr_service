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

#define LOG_TAG "KvStoreAppAccessor"

#include "kvstore_app_accessor.h"
#include <autils/constant.h>
#include <map>
#include "broadcast_sender.h"
#include "kv_store_delegate_manager.h"
#include "kvstore_meta_manager.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
using MetaDataManager = DistributedData::MetaDataManager;
using StoreMetaData = DistributedData::StoreMetaData;
using SecKeyMetaData = DistributedData::SecretKeyMetaData;
KvStoreAppAccessor::~KvStoreAppAccessor()
{
    ZLOGD("destructor.");
}

KvStoreAppAccessor::KvStoreAppAccessor()
{
    ZLOGD("constructor.");
}

KvStoreAppAccessor &KvStoreAppAccessor::GetInstance()
{
    static KvStoreAppAccessor appAccessor;
    return appAccessor;
}

void KvStoreAppAccessor::EnableKvStoreAutoLaunch(const AppAccessorParam &param)
{
    return;
}

void KvStoreAppAccessor::EnableKvStoreAutoLaunch()
{
    ZLOGI("start");
    return;
}

void KvStoreAppAccessor::DisableKvStoreAutoLaunch(const AppAccessorParam &param)
{
    (void) param;
}

void KvStoreAppAccessor::OnCallback(const std::string &userId, const std::string &appId, const std::string &storeId,
                                    const DistributedDB::AutoLaunchStatus status)
{
    ZLOGI("start");
    if (status == DistributedDB::AutoLaunchStatus::WRITE_OPENED) {
        KvStoreMetaData kvStoreMetaData;
        if (KvStoreMetaManager::GetInstance().GetKvStoreMetaDataByAppId(appId, kvStoreMetaData)) {
            BroadcastSender::GetInstance()->SendEvent({userId, kvStoreMetaData.bundleName, storeId});
            ZLOGW("AppId:%s is opened, bundle:%s.", appId.c_str(), kvStoreMetaData.bundleName.c_str());
        } else {
            ZLOGW("AppId:%s open failed.", appId.c_str());
        }
    } else {
        ZLOGD("AppId:%s is closed.", appId.c_str());
    }
}
} // namespace OHOS::DistributedKv

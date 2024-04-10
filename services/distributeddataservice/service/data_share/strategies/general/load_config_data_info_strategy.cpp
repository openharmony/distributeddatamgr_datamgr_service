/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "LoadConfigDataInfoStrategy"
#include "load_config_data_info_strategy.h"

#include "check_is_single_app_strategy.h"
#include "device_manager_adapter.h"
#include "extension_connect_adaptor.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "rdb_errno.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
LoadConfigDataInfoStrategy::LoadConfigDataInfoStrategy()
    : DivStrategy(std::make_shared<CheckIsSingleAppStrategy>(), std::make_shared<LoadConfigSingleDataInfoStrategy>(),
          std::make_shared<LoadConfigNormalDataInfoStrategy>())
{
}
static bool QueryMetaData(const std::string &bundleName, const std::string &storeName,
    DistributedData::StoreMetaData &metaData, const int32_t userId)
{
    DistributedData::StoreMetaData meta;
    meta.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(userId);
    meta.bundleName = bundleName;
    meta.storeId = storeName;
    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData, true);
    if (!isCreated) {
        ZLOGE("DB not exist");
        return false;
    }
    return true;
}

bool LoadConfigNormalDataInfoStrategy::operator()(std::shared_ptr<Context> context)
{
    if (context->type != "rdb") {
        return true;
    }
    DistributedData::StoreMetaData metaData;
    if (!QueryMetaData(context->calledBundleName, context->calledStoreName, metaData, context->currentUserId)) {
        // connect extension and retry
        ExtensionConnectAdaptor::TryAndWait(context->uri, context->calledBundleName);
        if (!QueryMetaData(context->calledBundleName, context->calledStoreName, metaData, context->currentUserId)) {
            ZLOGE("QueryMetaData fail, %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
            context->errCode = NativeRdb::E_DB_NOT_EXIST;
            return false;
        }
    }
    context->calledSourceDir = metaData.dataDir;
    context->isEncryptDb = metaData.isEncrypt;
    if (context->isEncryptDb) {
        context->secretMetaKey = metaData.GetSecretKey();
    }
    return true;
}

bool LoadConfigSingleDataInfoStrategy::operator()(std::shared_ptr<Context> context)
{
    DistributedData::StoreMetaData metaData;
    if (!QueryMetaData(context->calledBundleName, context->calledStoreName, metaData, 0)) {
        // connect extension and retry
        ExtensionConnectAdaptor::TryAndWait(context->uri, context->calledBundleName);
        if (!QueryMetaData(context->calledBundleName, context->calledStoreName, metaData, 0)) {
            ZLOGE("QueryMetaData fail, %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
            context->errCode = NativeRdb::E_DB_NOT_EXIST;
            return false;
        }
    }
    context->calledSourceDir = metaData.dataDir;
    return true;
}
} // namespace OHOS::DataShare
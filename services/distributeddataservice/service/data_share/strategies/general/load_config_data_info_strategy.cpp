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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "check_is_single_app_strategy.h"
#include "device_manager_adapter.h"
#include "extension_connect_adaptor.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "rdb_errno.h"
#include "tokenid_kit.h"
#include "utils.h"

namespace OHOS::DataShare {
LoadConfigDataInfoStrategy::LoadConfigDataInfoStrategy()
    : DivStrategy(std::make_shared<CheckIsSingleAppStrategy>(), std::make_shared<LoadConfigSingleDataInfoStrategy>(),
          std::make_shared<LoadConfigNormalDataInfoStrategy>())
{
}

static bool MatchAccountDataDir(const std::string &dataDir, int32_t accountId)
{
    if (accountId <= 0 || dataDir.empty()) {
        return false;
    }
    // dataDir ends with ".db", accountId is always a middle path segment followed by '/'.
    std::string token = "/" + std::to_string(accountId) + "/";
    return dataDir.find(token) != std::string::npos;
}

static bool QueryMetaData(std::shared_ptr<Context> context,
    const int32_t userId, DistributedData::StoreMetaData &metaData)
{
    DistributedData::StoreMetaMapping storeMetaMapping;
    storeMetaMapping.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaMapping.user = std::to_string(userId);
    storeMetaMapping.bundleName = context->calledBundleName;
    storeMetaMapping.storeId = context->calledStoreName;
    storeMetaMapping.instanceId = context->appIndex;

    if (context->accountIsolation && context->accountId > 0) {
        // No clone app + account isolation: prefix-query StoreMetaData for all same-name DBs
        // (key contains dataDir, one entry per DB), then match dataDir (DB path) by accountId.
        std::vector<DistributedData::StoreMetaData> metas;
        DistributedData::MetaDataManager::GetInstance().LoadMeta(storeMetaMapping.GetKeyWithoutPath(), metas, true);
        for (const auto &meta : metas) {
            if (MatchAccountDataDir(meta.dataDir, context->accountId)) {
                metaData = meta;
                return true;
            }
        }
        return false;
    }

    bool isCreated = DistributedData::MetaDataManager::GetInstance().LoadMeta(
        storeMetaMapping.GetKey(), storeMetaMapping, true);
    metaData = storeMetaMapping;
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
    if (!QueryMetaData(context, context->visitedUserId, metaData)) {
        // connect extension and retry
        AAFwk::WantParams wantParams;
        ExtensionConnectAdaptor::TryAndWait(context->uri, context->calledBundleName, wantParams);
        if (!QueryMetaData(context, context->visitedUserId, metaData)) {
            ZLOGE("QueryMetaData fail, %{public}s", URIUtils::Anonymous(context->uri).c_str());
            context->errCode = NativeRdb::E_DB_NOT_EXIST;
            return false;
        }
    }
    context->calledSourceDir = metaData.dataDir;
    context->isEncryptDb = metaData.isEncrypt;
    context->calledTokenId = metaData.tokenId;
    context->calledStoreName = metaData.storeId;
    context->haMode = metaData.haMode;
    if (context->isEncryptDb) {
        context->secretMetaKey = metaData.GetSecretKey();
    }
    return true;
}

bool LoadConfigSingleDataInfoStrategy::operator()(std::shared_ptr<Context> context)
{
    DistributedData::StoreMetaData metaData;
    if (!QueryMetaData(context, 0, metaData)) {
        // connect extension and retry
        AAFwk::WantParams wantParams;
        ExtensionConnectAdaptor::TryAndWait(context->uri, context->calledBundleName, wantParams);
        if (!QueryMetaData(context, 0, metaData)) {
            ZLOGE("QueryMetaData fail, %{public}s", URIUtils::Anonymous(context->uri).c_str());
            context->errCode = NativeRdb::E_DB_NOT_EXIST;
            return false;
        }
    }
    context->calledSourceDir = metaData.dataDir;
    context->calledTokenId = metaData.tokenId;
    context->calledStoreName = metaData.storeId;
    context->haMode = metaData.haMode;
    return true;
}
} // namespace OHOS::DataShare
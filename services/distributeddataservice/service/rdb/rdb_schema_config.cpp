/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "RdbSchemaConfig"

#include "rdb_schema_config.h"
#include <fstream>
#include <sstream>
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "metadata/meta_data_manager.h"
#include "iremote_broker.h"
#include "iremote_object.h"
#include "refbase.h"
#include "resource_manager.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::Global::Resource;
using namespace OHOS::DistributedData;
using Serializable = DistributedData::Serializable;
constexpr const char *SCHEMA_PATH = "arkdata/schema/sync_schema.json";

bool RdbSchemaConfig::GetDistributedSchema(const StoreMetaData &meta, Database &database)
{
    OHOS::AppExecFwk::BundleInfo bundleInfo;
    if (!InitBundleInfo(meta.bundleName, std::atoi(meta.user.c_str()), bundleInfo)) {
        return false;
    }
    auto ret = GetSchemaFromHap(bundleInfo, meta.storeId, meta.bundleName, database);
    if (ret) {
        database.user = meta.user;
        database.deviceId = meta.deviceId;
        return true;
    }
    return false;
}

bool RdbSchemaConfig::InitBundleInfo(
    const std::string &bundleName, int32_t userId, OHOS::AppExecFwk::BundleInfo &bundleInfo)
{
    OHOS::sptr<OHOS::ISystemAbilityManager> systemAbilityManager =
        OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Fail to get ststem ability mgr, bundleName: %{public}s.", bundleName.c_str());
        return false;
    }

    OHOS::sptr<OHOS::IRemoteObject> remoteObject =
        systemAbilityManager->GetSystemAbility(OHOS::BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        ZLOGE("Fail to get bundle manager proxy, bundleName: %{public}s.", bundleName.c_str());
        return false;
    }

    auto bundleMgrProxy = iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if (bundleMgrProxy == nullptr) {
        ZLOGE("Fail to cast proxy, bundleName: %{public}s.", bundleName.c_str());
        return false;
    }
    bool ret = bundleMgrProxy->GetBundleInfo(
        bundleName, OHOS::AppExecFwk::BundleFlag::GET_BUNDLE_WITH_EXTENSION_INFO, bundleInfo, userId);
    if (!ret || bundleInfo.moduleDirs.size() == 0) {
        ZLOGE("Get bundle info failed, bundleName: %{public}s.", bundleName.c_str());
        return false;
    }
    return true;
}

bool RdbSchemaConfig::GetSchemaFromHap(const OHOS::AppExecFwk::BundleInfo &bundleInfo, const std::string &storeName,
    const std::string &bundleName, Database &database)
{
    for (auto &hapInfo : bundleInfo.hapModuleInfos) {
        std::shared_ptr<ResourceManager> resMgr(CreateResourceManager());
        if (resMgr == nullptr) {
            ZLOGE("Create resourceManager failed");
            return false;
        }
        resMgr->AddResource(hapInfo.hapPath.c_str());
        size_t length = 0;
        std::unique_ptr<uint8_t[]> fileContent;
        int err = resMgr->GetRawFileFromHap(SCHEMA_PATH, length, fileContent);
        if (err != 0) {
            continue;
        }
        std::string jsonData(fileContent.get(), fileContent.get() + length);
        DbSchema databases;
        databases.Unmarshall(jsonData);
        for (const auto &schema : databases.databases) {
            if (schema.name == storeName && schema.bundleName == bundleName) {
                database = schema;
                return true;
            }
        }
    }
    return false;
}
}  // namespace OHOS::DistributedRdb
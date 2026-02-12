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
#define LOG_TAG "DataShareDbConfig"

#include "data_share_db_config.h"

#include <tuple>
#include <utility>

#include "datashare_errno.h"
#include "datashare_radar_reporter.h"
#include "device_manager_adapter.h"
#include "extension_connect_adaptor.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "utils.h"
#include "utils/anonymous.h"
#include "ipc_skeleton.h"

namespace OHOS::DataShare {
static const int32_t WAIT_TIME = 2;
std::pair<bool, DistributedData::StoreMetaData> DataShareDbConfig::QueryMetaData(
    const std::string &bundleName, const std::string &storeName, int32_t userId, int32_t appIndex)
{
    TimeoutReport timeoutReport({bundleName, "", storeName, __FUNCTION__, 0});
    DistributedData::StoreMetaMapping storeMetaMapping;
    storeMetaMapping.deviceId = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaMapping.user = std::to_string(userId);
    storeMetaMapping.bundleName = bundleName;
    storeMetaMapping.storeId = storeName;
    storeMetaMapping.instanceId = appIndex;
    bool isCreated =
        DistributedData::MetaDataManager::GetInstance().LoadMeta(storeMetaMapping.GetKey(), storeMetaMapping, true);

    auto callingPid = IPCSkeleton::GetCallingPid();
    timeoutReport.Report(std::to_string(userId), callingPid, appIndex);
    return std::make_pair(isCreated, storeMetaMapping);
}

std::pair<int, DistributedData::StoreMetaData> DataShareDbConfig::GetMetaData(const DbConfig &dbConfig)
{
    auto [success, metaData] = QueryMetaData(
        dbConfig.bundleName, dbConfig.storeName, dbConfig.userId, dbConfig.appIndex);
    if (!success) {
        // without extension configuration, load the SA when there is no metadata information.
        int32_t systemAbilityId = URIUtils::GetSystemAbilityId(dbConfig.uri);
        if (systemAbilityId != URIUtils::INVALID_SA_ID) {
            ZLOGE("QueryMeta failed, checkAndLoad %{public}d", systemAbilityId);
            auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
            if (manager == nullptr) {
                ZLOGE("get system ability manager failed");
                return std::pair(NativeRdb::E_DB_NOT_EXIST, metaData);
            }
            auto remoteObject = manager->CheckSystemAbility(systemAbilityId);
            if (remoteObject == nullptr) {
                // check SA failed, try load SA
                remoteObject = manager->LoadSystemAbility(systemAbilityId, WAIT_TIME);
            }
            // load failed or timeout, return E_DB_NOT_EXIST
            if (remoteObject == nullptr) {
                ZLOGE("load systemAbility:%{public}d failed", systemAbilityId);
                return std::pair(NativeRdb::E_DB_NOT_EXIST, metaData);
            }
        } else {
            if (!dbConfig.hasExtension) {
                return std::pair(NativeRdb::E_DB_NOT_EXIST, metaData);
            }
            AAFwk::WantParams wantParams;
            ExtensionConnectAdaptor::TryAndWait(dbConfig.uri, dbConfig.bundleName, wantParams);
        }
        auto [succ, meta] = QueryMetaData(dbConfig.bundleName, dbConfig.storeName, dbConfig.userId, dbConfig.appIndex);
        if (!succ) {
            return std::pair(NativeRdb::E_DB_NOT_EXIST, meta);
        }
        metaData = std::move(meta);
    }
    return std::pair(E_OK, metaData);
}

std::tuple<int, DistributedData::StoreMetaData, std::shared_ptr<DBDelegate>> DataShareDbConfig::GetDbConfig(
    DbConfig &dbConfig)
{
    auto [errCode, metaData] = GetMetaData(dbConfig);
    if (errCode != E_OK) {
        ZLOGE("DB not exist,bundleName:%{public}s,storeName:%{public}s,user:%{public}d,err:%{public}d,uri:%{public}s",
            dbConfig.bundleName.c_str(), StringUtils::GeneralAnonymous(dbConfig.storeName).c_str(), dbConfig.userId,
            errCode, URIUtils::Anonymous(dbConfig.uri).c_str());
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_MATEDATA_EXISTS,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::META_DATA_NOT_EXISTS);
        return std::make_tuple(errCode, metaData, nullptr);
    }
    auto dbDelegate = DBDelegate::Create(metaData, dbConfig.extUri, dbConfig.backup);
    if (dbDelegate == nullptr) {
        ZLOGE("Create delegate fail, bundleName:%{public}s, userId:%{public}d, uri:%{public}s",
            dbConfig.bundleName.c_str(), dbConfig.userId, URIUtils::Anonymous(dbConfig.uri).c_str());
        return std::make_tuple(E_ERROR, metaData, nullptr);
    }
    return std::make_tuple(E_OK, metaData, dbDelegate);
}
} // namespace OHOS::DataShare

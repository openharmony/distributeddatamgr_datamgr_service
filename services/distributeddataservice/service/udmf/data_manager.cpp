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
#define LOG_TAG "DataManager"

#include "data_manager.h"

#include "checker_manager.h"
#include "dfx_types.h"
#include "file.h"
#include "lifecycle/lifecycle_manager.h"
#include "log_print.h"
#include "preprocess_utils.h"
#include "uri_permission_manager.h"
#include "uri.h"

namespace OHOS {
namespace UDMF {
const std::string MSDP_PROCESS_NAME = "msdp_sa";
const std::string DATA_PREFIX = "udmf://";
DataManager::DataManager()
{
    authorizationMap_[UD_INTENTION_MAP.at(UD_INTENTION_DRAG)] = MSDP_PROCESS_NAME;
    CheckerManager::GetInstance().LoadCheckers();
}

DataManager::~DataManager()
{
}

DataManager &DataManager::GetInstance()
{
    static DataManager instance;
    return instance;
}

int32_t DataManager::SaveData(CustomOption &option, UnifiedData &unifiedData, std::string &key)
{
    if (unifiedData.IsEmpty()) {
        ZLOGE("Invalid parameters, have no record");
        return E_INVALID_PARAMETERS;
    }

    if (!UnifiedDataUtils::IsValidIntention(option.intention)) {
        ZLOGE("Invalid parameters intention: %{public}d.", option.intention);
        return E_INVALID_PARAMETERS;
    }

    // imput runtime info before put it into store and save one privilege
    if (PreProcessUtils::RuntimeDataImputation(unifiedData, option) != E_OK) {
        ZLOGE("Imputation failed");
        return E_UNKNOWN;
    }

    std::string intention = unifiedData.GetRuntime()->key.intention;
    if (intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        int32_t ret = PreProcessUtils::SetRemoteUri(option.tokenId, unifiedData);
        if (ret != E_OK) {
            ZLOGE("SetRemoteUri failed, ret: %{public}d.", ret);
            return ret;
        }
    }

    for (const auto &record : unifiedData.GetRecords()) {
        record->SetUid(PreProcessUtils::IdGenerator());
    }

    auto store = storeCache_.GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    if (!UnifiedDataUtils::IsPersist(intention) && store->Clear() != E_OK) {
        ZLOGE("Clear store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    if (store->Put(unifiedData) != E_OK) {
        ZLOGE("Put unified data failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }
    key = unifiedData.GetRuntime()->key.GetUnifiedKey();
    ZLOGD("Put unified data successful, key: %{public}s.", key.c_str());
    return E_OK;
}

int32_t DataManager::RetrieveData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto store = storeCache_.GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    int32_t res = store->Get(query.key, unifiedData);
    if (res != E_OK) {
        ZLOGE("Get data from store failed, res: %{public}d, key: %{public}s.", res, query.key.c_str());
        return res;
    }

    std::shared_ptr<Runtime> runtime = unifiedData.GetRuntime();
    if (static_cast<uint32_t>(unifiedData.GetRecords().size()) != runtime->recordTotalNum) {
        ZLOGE("Get data from DB is incomplete, key: %{public}s, expected recordsNum is %{public}u, not %{public}zu.",
              query.key.c_str(), runtime->recordTotalNum, unifiedData.GetRecords().size());
        return E_NOT_FOUND;
    }

    CheckerManager::CheckInfo info;
    info.tokenId = query.tokenId;
    if (!CheckerManager::GetInstance().IsValid(runtime->privileges, info) && !CheckPermissionInCache(query)) {
        return E_NO_PERMISSION;
    }

    if (key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        int32_t ret = ProcessingUri(query, unifiedData);
        if (ret != E_OK) {
            ZLOGE("DragUriProcessing failed. ret=%{public}d", ret);
            return E_NO_PERMISSION;
        }
    }

    if (LifeCycleManager::GetInstance().DeleteOnGet(key) != E_OK) {
        ZLOGE("Remove data failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    privilegeCache_.erase(query.key);

    PreProcessUtils::SetRemoteData(unifiedData);
    return E_OK;
}

bool DataManager::CheckPermissionInCache(const QueryOption &query)
{
    auto iter = privilegeCache_.find(query.key);
    if (iter != privilegeCache_.end() && iter->second.tokenId == query.tokenId) {
        return true;
    }
    return false;
}

int32_t DataManager::ProcessingUri(const QueryOption &query, UnifiedData &unifiedData)
{
    std::string localDeviceId = PreProcessUtils::GetLocalDeviceId();
    auto records = unifiedData.GetRecords();
    if (localDeviceId != unifiedData.GetRuntime()->deviceId) {
        for (auto record : records) {
            if (record != nullptr && PreProcessUtils::IsFileType(record->GetType())) {
                auto file = static_cast<File *>(record.get());
                std::string remoteUri = file->GetRemoteUri();
                if (remoteUri.empty()) {
                    ZLOGW("Get remoteUri is empyt, key=%{public}s.", query.key.c_str());
                    continue;
                }
                file->SetUri(remoteUri); // cross dev, need dis path.
            }
        }
    }

    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        ZLOGE("GetHapBundleNameByToken fail, key=%{public}s.", query.key.c_str());
        return E_ERROR;
    }
    for (auto record : records) {
        if (record != nullptr && PreProcessUtils::IsFileType(record->GetType())) {
            auto file = static_cast<File *>(record.get());
            if (file->GetUri().empty()) {
                ZLOGW("Get uri is empty, key=%{public}s.", query.key.c_str());
                continue;
            }
            Uri uri(file->GetUri());
            if (uri.GetAuthority().empty()) {
                ZLOGW("Get authority is empty, key=%{public}s.", query.key.c_str());
                continue;
            }
            if (UriPermissionManager::GetInstance().GrantUriPermission(file->GetUri(), bundleName) != E_OK) {
                ZLOGE("GrantUriPermission fail, uriAuthority=%{public}s, bundleName=%{public}s, key=%{public}s.",
                      uri.GetAuthority().c_str(), bundleName.c_str(), query.key.c_str());
                return E_NO_PERMISSION;
            }
        }
    }
    return E_OK;
}

int32_t DataManager::RetrieveBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    std::vector<UnifiedData> dataSet;
    std::shared_ptr<Store> store;
    auto status = QueryDataCommon(query, dataSet, store);
    if (status != E_OK) {
        ZLOGE("QueryDataCommon failed.");
        return status;
    }
    if (dataSet.empty()) {
        ZLOGW("DataSet has no data, key: %{public}s, intention: %{public}d.", query.key.c_str(), query.intention);
        return E_OK;
    }
    for (auto &data : dataSet) {
        PreProcessUtils::SetRemoteData(data);
        unifiedDataSet.push_back(data);
    }
    return E_OK;
}

int32_t DataManager::UpdateData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    if (unifiedData.IsEmpty()) {
        ZLOGE("Invalid parameters, unified data has no record.");
        return E_INVALID_PARAMETERS;
    }
    auto store = storeCache_.GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData data;
    int32_t res = store->Get(query.key, data);
    if (res != E_OK) {
        ZLOGE("Get data from store failed, intention: %{public}s.", key.intention.c_str());
        return res;
    }
    if (data.IsEmpty()) {
        ZLOGE("Invalid parameter, unified data has no record; intention: %{public}s.", key.intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::shared_ptr<Runtime> runtime = data.GetRuntime();
    runtime->lastModifiedTime = PreProcessUtils::GetTimeStamp();
    unifiedData.SetRuntime(*runtime);
    for (auto &record : unifiedData.GetRecords()) {
        record->SetUid(PreProcessUtils::IdGenerator());
    }
    if (store->Update(unifiedData) != E_OK) {
        ZLOGE("Update unified data failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}
int32_t DataManager::DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    std::vector<UnifiedData> dataSet;
    std::shared_ptr<Store> store;
    auto status = QueryDataCommon(query, dataSet, store);
    if (status != E_OK) {
        ZLOGE("QueryDataCommon failed.");
        return status;
    }
    if (dataSet.empty()) {
        ZLOGW("DataSet has no data, key: %{public}s, intention: %{public}d.", query.key.c_str(), query.intention);
        return E_OK;
    }
    std::shared_ptr<Runtime> runtime;
    std::vector<std::string> deleteKeys;
    for (const auto &data : dataSet) {
        runtime = data.GetRuntime();
        unifiedDataSet.push_back(data);
        deleteKeys.push_back(runtime->key.key);
    }
    if (store->DeleteBatch(deleteKeys) != E_OK) {
        ZLOGE("Remove data failed.");
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t DataManager::GetSummary(const QueryOption &query, Summary &summary)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = storeCache_.GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }

    if (store->GetSummary(query.key, summary) != E_OK) {
        ZLOGE("Store get summary failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t DataManager::AddPrivilege(const QueryOption &query, const Privilege &privilege)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    std::string processName;
    if (!PreProcessUtils::GetNativeProcessNameByToken(query.tokenId, processName)) {
        return E_UNKNOWN;
    }

    if (processName != authorizationMap_[key.intention]) {
        ZLOGE("Process: %{public}s have no permission", processName.c_str());
        return E_NO_PERMISSION;
    }

    auto store = storeCache_.GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData data;
    int32_t res = store->Get(query.key, data);
    if (res == E_NOT_FOUND) {
        privilegeCache_[query.key] = privilege;
        ZLOGW("Add privilege in cache, key: %{public}s.", query.key.c_str());
        return E_OK;
    }
    if (res != E_OK) {
        ZLOGE("Get data from store failed, res:%{public}d,intention: %{public}s.", res, key.intention.c_str());
        return res;
    }
    data.GetRuntime()->privileges.emplace_back(privilege);
    if (store->Update(data) != E_OK) {
        ZLOGE("Update unified data failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t DataManager::Sync(const QueryOption &query, const std::vector<std::string> &devices)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = storeCache_.GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }

    if (store->Sync(devices) != E_OK) {
        ZLOGE("Store sync failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t DataManager::QueryDataCommon(
    const QueryOption &query, std::vector<UnifiedData> &dataSet, std::shared_ptr<Store> &store)
{
    auto find = UD_INTENTION_MAP.find(query.intention);
    std::string intention = find == UD_INTENTION_MAP.end() ? intention : find->second;
    if (!UnifiedDataUtils::IsValidOptions(query.key, intention)) {
        ZLOGE("Unified key: %{public}s and intention: %{public}s is invalid.", query.key.c_str(), intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string dataPrefix = DATA_PREFIX + intention;
    UnifiedKey key(query.key);
    key.IsValid();
    if (intention.empty()) {
        dataPrefix = key.key;
        intention = key.intention;
    }
    ZLOGD("dataPrefix = %{public}s, intention: %{public}s.", dataPrefix.c_str(), intention.c_str());
    store = storeCache_.GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }
    if (store->GetBatchData(dataPrefix, dataSet) != E_OK) {
        ZLOGE("Get dataSet failed, dataPrefix: %{public}s.", dataPrefix.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
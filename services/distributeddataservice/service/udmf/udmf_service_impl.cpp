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

#define LOG_TAG "UdmfServiceImpl"

#include "udmf_service_impl.h"

#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"

#include "accesstoken_kit.h"
#include "checker_manager.h"
#include "dfx_types.h"
#include "distributed_kv_data_manager.h"
#include "file.h"
#include "lifecycle/lifecycle_manager.h"
#include "log_print.h"
#include "preprocess_utils.h"
#include "reporter.h"
#include "uri_permission_manager.h"
#include "uri.h"
#include "udmf_conversion.h"
#include "udmf_radar_reporter.h"
#include "securec.h"
#include "unified_types.h"
#include "device_manager_adapter.h"
#include "store_account_observer.h"


namespace OHOS {
namespace UDMF {
using namespace Security::AccessToken;
using FeatureSystem = DistributedData::FeatureSystem;
using UdmfBehaviourMsg = OHOS::DistributedDataDfx::UdmfBehaviourMsg;
using Reporter = OHOS::DistributedDataDfx::Reporter;
using namespace RadarReporter;
using namespace DistributedKv;
constexpr const char *DRAG_AUTHORIZED_PROCESSES[] = {"msdp_sa", "collaboration_service"};
constexpr const char *DATA_PREFIX = "udmf://";
constexpr const char *FILE_SCHEME = "file";
constexpr const char *PRIVILEGE_READ_AND_KEEP = "readAndKeep";
__attribute__((used)) UdmfServiceImpl::Factory UdmfServiceImpl::factory_;
UdmfServiceImpl::Factory::Factory()
{
    ZLOGI("Register udmf creator!");
    FeatureSystem::GetInstance().RegisterCreator("udmf", [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<UdmfServiceImpl>();
        }
        return product_;
    }, FeatureSystem::BIND_NOW);
    auto observer = std::make_shared<RuntimeStoreAccountObserver>();
    DistributedKv::AccountDelegate::GetInstance()->Subscribe(observer);
}

UdmfServiceImpl::Factory::~Factory()
{
    product_ = nullptr;
}

UdmfServiceImpl::UdmfServiceImpl()
{
    CheckerManager::GetInstance().LoadCheckers();
}

int32_t UdmfServiceImpl::SetData(CustomOption &option, UnifiedData &unifiedData, std::string &key)
{
    ZLOGD("start");
    int32_t res = E_OK;
    UdmfBehaviourMsg msg;
    std::string types;
    auto find = UD_INTENTION_MAP.find(option.intention);
    msg.channel = find == UD_INTENTION_MAP.end() ? "invalid" : find->second;
    msg.operation = "insert";
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(option.tokenId, bundleName)) {
        msg.appId = "unknown";
        res = E_ERROR;
    } else {
        msg.appId = bundleName;
        res = SaveData(option, unifiedData, key);
    }
    auto errFind = ERROR_MAP.find(res);
    msg.result = errFind == ERROR_MAP.end() ? "E_ERROR" : errFind->second;

    for (const auto &record : unifiedData.GetRecords()) {
        for (const auto &type : record->GetUtdIds()) {
            types.append("-").append(type);
        }
    }
    msg.dataType = types;
    msg.dataSize = unifiedData.GetSize();
    Reporter::GetInstance()->BehaviourReporter()->UDMFReport(msg);
    return res;
}

int32_t UdmfServiceImpl::SaveData(CustomOption &option, UnifiedData &unifiedData, std::string &key)
{
    if (!unifiedData.IsValid()) {
        ZLOGE("UnifiedData is invalid.");
        return E_INVALID_PARAMETERS;
    }

    if (!UnifiedDataUtils::IsValidIntention(option.intention)) {
        ZLOGE("Invalid parameters intention: %{public}d.", option.intention);
        return E_INVALID_PARAMETERS;
    }

    // imput runtime info before put it into store and save one privilege
    if (PreProcessUtils::RuntimeDataImputation(unifiedData, option) != E_OK) {
        ZLOGE("Imputation failed");
        return E_ERROR;
    }

    std::string intention = unifiedData.GetRuntime()->key.intention;
    if (intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        int32_t ret = PreProcessUtils::SetRemoteUri(option.tokenId, unifiedData);
        if (ret != E_OK) {
            ZLOGE("SetRemoteUri failed, ret: %{public}d, bundleName:%{public}s.", ret,
                  unifiedData.GetRuntime()->createPackage.c_str());
            return ret;
        }
    }

    for (const auto &record : unifiedData.GetRecords()) {
        record->SetUid(PreProcessUtils::GenerateId());
    }

    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    if (!UnifiedDataUtils::IsPersist(intention) && store->Clear() != E_OK) {
        ZLOGE("Clear store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    UdmfConversion::InitValueObject(unifiedData);
    if (store->Put(unifiedData) != E_OK) {
        ZLOGE("Put unified data failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }
    key = unifiedData.GetRuntime()->key.GetUnifiedKey();
    ZLOGD("Put unified data successful, key: %{public}s.", key.c_str());
    return E_OK;
}

int32_t UdmfServiceImpl::GetData(const QueryOption &query, UnifiedData &unifiedData)
{
    ZLOGD("start");
    int32_t res = E_OK;
    UdmfBehaviourMsg msg;
    std::string types;
    auto find = UD_INTENTION_MAP.find(query.intention);
    msg.channel = find == UD_INTENTION_MAP.end() ? "invalid" : find->second;
    msg.operation = "insert";
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        msg.appId = "unknown";
        res = E_ERROR;
    } else {
        msg.appId = bundleName;
        res = RetrieveData(query, unifiedData);
    }
    auto errFind = ERROR_MAP.find(res);
    msg.result = errFind == ERROR_MAP.end() ? "E_ERROR" : errFind->second;
    for (const auto &record : unifiedData.GetRecords()) {
        for (const auto &type : record->GetUtdIds()) {
            types.append("-").append(type);
        }
    }
    msg.dataType = types;
    msg.dataSize = unifiedData.GetSize();
    Reporter::GetInstance()->BehaviourReporter()->UDMFReport(msg);
    return res;
}

int32_t UdmfServiceImpl::RetrieveData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    int32_t res = store->Get(query.key, unifiedData);
    if (res != E_OK) {
        ZLOGE("Get data from store failed, res: %{public}d, key: %{public}s.", res, query.key.c_str());
        return res;
    }

    if (!unifiedData.IsComplete()) {
        ZLOGE("Get data from DB is incomplete, key: %{public}s.", query.key.c_str());
        return E_NOT_FOUND;
    }

    CheckerManager::CheckInfo info;
    info.tokenId = query.tokenId;
    std::shared_ptr<Runtime> runtime = unifiedData.GetRuntime();
    if (runtime == nullptr) {
        return E_DB_ERROR;
    }
    if (!CheckerManager::GetInstance().IsValid(runtime->privileges, info) && !IsPermissionInCache(query)) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::GET_DATA, GetDataStage::VERIFY_PRIVILEGE, StageRes::FAILED, E_NO_PERMISSION);
        return E_NO_PERMISSION;
    }

    if (key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        int32_t ret = ProcessUri(query, unifiedData);
        if (ret != E_OK) {
            ZLOGE("ProcessUri failed. ret=%{public}d", ret);
            return E_NO_PERMISSION;
        }
    }
    if (!IsReadAndKeep(runtime->privileges, query)) {
        if (LifeCycleManager::GetInstance().OnGot(key) != E_OK) {
            ZLOGE("Remove data failed, intention: %{public}s.", key.intention.c_str());
            return E_DB_ERROR;
        }
    }

    privilegeCache_.erase(query.key);

    PreProcessUtils::SetRemoteData(unifiedData);
    return E_OK;
}

bool UdmfServiceImpl::IsPermissionInCache(const QueryOption &query)
{
    auto iter = privilegeCache_.find(query.key);
    if (iter != privilegeCache_.end() && iter->second.tokenId == query.tokenId) {
        return true;
    }
    return false;
}

bool UdmfServiceImpl::IsReadAndKeep(const std::vector<Privilege> &privileges, const QueryOption &query)
{
    for (const auto &privilege : privileges) {
        if (privilege.tokenId == query.tokenId && privilege.readPermission == PRIVILEGE_READ_AND_KEEP) {
            return true;
        }
    }
    
    auto iter = privilegeCache_.find(query.key);
    if (iter != privilegeCache_.end() && iter->second.tokenId == query.tokenId &&
        iter->second.readPermission == PRIVILEGE_READ_AND_KEEP) {
        return true;
    }
    return false;
}

int32_t UdmfServiceImpl::ProcessUri(const QueryOption &query, UnifiedData &unifiedData)
{
    std::string localDeviceId = PreProcessUtils::GetLocalDeviceId();
    int32_t verifyRes = VerifyUnifiedData(unifiedData);
    if (verifyRes != E_OK) {
        ZLOGE("verify unifieddata fail, key=%{public}s, stauts=%{public}d", query.key.c_str(), verifyRes);
        return verifyRes;
    }
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        ZLOGE("GetHapBundleNameByToken fail, key=%{public}s, tokenId=%{private}d.", query.key.c_str(), query.tokenId);
        return E_ERROR;
    }
    std::string sourceDeviceId = unifiedData.GetRuntime()->deviceId;
    if (localDeviceId == sourceDeviceId && query.tokenId == unifiedData.GetRuntime()->tokenId) {
        ZLOGW("No need to grant uri permissions, queryKey=%{public}s.", query.key.c_str());
        return E_OK;
    }
    std::vector<Uri> allUri;
    auto records = unifiedData.GetRecords();
    for (auto record : records) {
        if (record != nullptr && PreProcessUtils::IsFileType(record->GetType())) {
            auto file = static_cast<File *>(record.get());
            if (file->GetUri().empty()) {
                ZLOGW("Get uri is empty, key=%{public}s.", query.key.c_str());
                continue;
            }
            Uri uri(file->GetUri());
            std::string scheme = uri.GetScheme();
            std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
            if (uri.GetAuthority().empty() || scheme != FILE_SCHEME) {
                ZLOGW("Get authority is empty or uri scheme not equals to file, key=%{public}s.", query.key.c_str());
                continue;
            }
            allUri.push_back(uri);
        }
    }
    asyncProcessInfo_.permStatus = ASYNC_RUNNING;
    asyncProcessInfo_.permTotal = allUri.size();
    if (UriPermissionManager::GetInstance().GrantUriPermission(allUri, query.tokenId, query.key,
        asyncProcessInfo_.permFnished) != E_OK) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::GET_DATA, GetDataStage::GRANT_URI_PERMISSION, StageRes::FAILED, E_NO_PERMISSION);
        ZLOGE("GrantUriPermission fail, bundleName=%{public}s, key=%{public}s.",
            bundleName.c_str(), query.key.c_str());
        return E_NO_PERMISSION;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::VerifyUnifiedData(UnifiedData &unifiedData)
{
    if (unifiedData.GetRuntime() == nullptr) {
        ZLOGE("get runtime empty!");
        return E_DB_ERROR;
    }
    std::string localDeviceId = PreProcessUtils::GetLocalDeviceId();
    std::string sourceDeviceId = unifiedData.GetRuntime()->deviceId;
    if (localDeviceId == sourceDeviceId) {
        return E_OK;
    }
    auto records = unifiedData.GetRecords();
    for (auto record : records) {
        if (record != nullptr && PreProcessUtils::IsFileType(record->GetType())) {
            auto file = static_cast<File *>(record.get());
            std::string remoteUri = file->GetRemoteUri();
            if (remoteUri.empty()) {
                ZLOGE("when cross devices, remote uri is required!");
                return E_ERROR;
            }
            file->SetUri(remoteUri); // cross dev, need dis path.
        }
    }
    return E_OK;
}

int32_t UdmfServiceImpl::GetBatchData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    ZLOGD("start");
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

int32_t UdmfServiceImpl::UpdateData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!unifiedData.IsValid() || !key.IsValid()) {
        ZLOGE("data is invalid, or key is invalid. key=%{public}s.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string bundleName;
    PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName);
    if (key.bundleName != bundleName) {
        ZLOGE("update data failed by %{public}s, key: %{public}s.", bundleName.c_str(), query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto store = StoreCache::GetInstance().GetStore(key.intention);
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
    if (runtime == nullptr) {
        return E_DB_ERROR;
    }
    if (runtime->tokenId != query.tokenId) {
        ZLOGE("update data failed, query option tokenId not equals data's tokenId");
        return E_INVALID_PARAMETERS;
    }
    runtime->lastModifiedTime = PreProcessUtils::GetTimestamp();
    unifiedData.SetRuntime(*runtime);
    for (auto &record : unifiedData.GetRecords()) {
        record->SetUid(PreProcessUtils::GenerateId());
    }
    UdmfConversion::InitValueObject(unifiedData);
    if (store->Update(unifiedData) != E_OK) {
        ZLOGE("Update unified data failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    ZLOGD("start");
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
        if (runtime == nullptr) {
            return E_DB_ERROR;
        }
        if (runtime->tokenId == query.tokenId) {
            unifiedDataSet.push_back(data);
            deleteKeys.push_back(runtime->key.key);
        }
    }
    if (deleteKeys.empty()) {
        ZLOGE("Delete nothing. There is no data belonging to this application");
        return E_OK;
    }
    ZLOGI("Delete data start. size: %{public}zu.", deleteKeys.size());
    if (store->DeleteBatch(deleteKeys) != E_OK) {
        ZLOGE("Remove data failed.");
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::GetSummary(const QueryOption &query, Summary &summary)
{
    ZLOGD("start");
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
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

int32_t UdmfServiceImpl::AddPrivilege(const QueryOption &query, Privilege &privilege)
{
    ZLOGD("start");
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    std::string processName;
    if (!PreProcessUtils::GetNativeProcessNameByToken(query.tokenId, processName)) {
        return E_ERROR;
    }

    if (key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        if (find(DRAG_AUTHORIZED_PROCESSES, std::end(DRAG_AUTHORIZED_PROCESSES), processName) ==
            std::end(DRAG_AUTHORIZED_PROCESSES)) {
            ZLOGE("Process: %{public}s has no permission to intention: drag", processName.c_str());
            return E_NO_PERMISSION;
        }
    } else {
        ZLOGE("Intention: %{public}s has no authorized processes", key.intention.c_str());
        return E_NO_PERMISSION;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
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
    if (data.GetRuntime() == nullptr) {
        return E_DB_ERROR;
    }
    data.GetRuntime()->privileges.emplace_back(privilege);
    UdmfConversion::InitValueObject(data);
    if (store->Update(data) != E_OK) {
        ZLOGE("Update unified data failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::Sync(const QueryOption &query, const std::vector<std::string> &devices)
{
    ZLOGD("start");
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::IDLE, BizState::DFX_BEGIN);
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    syncingData_ = true;
    if (devices.size() > 0) {
        syncingDevName_ = DistributedData::DeviceManagerAdapter::GetInstance().GetDeviceInfo(devices[0]).deviceName;
    }
    auto callback = [this](AsyncProcessInfo &syncInfo) {
        asyncProcessInfo_.syncId = syncInfo.syncId;
        asyncProcessInfo_.syncStatus = syncInfo.syncStatus;
        asyncProcessInfo_.syncTotal = syncInfo.syncTotal;
        asyncProcessInfo_.syncFinished = syncInfo.syncFinished;
        asyncProcessInfo_.srcDevName = syncInfo.srcDevName;
        if (asyncProcessInfo_.syncStatus != ASYNC_RUNNING) {
            syncingData_ = false;
        }
        ZLOGD("store.Sync: name=%{public}s, id=%{public}u, status=%{public}u, total=%{public}u, finish=%{public}u",
            syncInfo.srcDevName.c_str(), syncInfo.syncId, syncInfo.syncStatus,
            syncInfo.syncTotal, syncInfo.syncFinished);
    };
    if (store->Sync(devices, callback) != E_OK) {
        syncingData_ = false;
        ZLOGE("Store sync failed, intention: %{public}s.", key.intention.c_str());
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, E_DB_ERROR, BizState::DFX_ABNORMAL_END);
        return E_DB_ERROR;
    }
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::SUCCESS, BizState::DFX_NORMAL_END);
    return E_OK;
}

int32_t UdmfServiceImpl::IsRemoteData(const QueryOption &query, bool &result)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData unifiedData;
    if (store->Get(query.key, unifiedData) != E_OK) {
        ZLOGE("Store get unifiedData failed, intention: %{public}s.", key.intention.c_str());
        return E_DB_ERROR;
    }
    std::shared_ptr<Runtime> runtime = unifiedData.GetRuntime();
    if (runtime == nullptr) {
        ZLOGE("Store get runtime failed, key: %{public}s.", query.key.c_str());
        return E_DB_ERROR;
    }

    std::string localDeviceId = PreProcessUtils::GetLocalDeviceId();
    if (localDeviceId != runtime->deviceId) {
        result = true;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::SetAppShareOption(const std::string &intention, int32_t shareOption)
{
    if (intention.empty() || shareOption >= SHARE_OPTIONS_BUTT || shareOption < IN_APP) {
        ZLOGE("SetAppShareOption : para is invalid, intention: %{public}s, shareOption:%{public}d.",
              intention.c_str(), shareOption);
        return E_INVALID_PARAMETERS;
    }

    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    if (!isSystemApp) {
        ZLOGE("no system permission, intention: %{public}s.", intention.c_str());
        return E_NO_PERMISSION;
    }
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    std::string shareOptionTmp;
    if (store->GetLocal(std::to_string(accessTokenIDEx), shareOptionTmp) == E_OK) {
        ZLOGE("SetAppShareOption failed, shareOption has already been set, %{public}s.", shareOptionTmp.c_str());
        return E_SETTINGS_EXISTED;
    }

    if (store->PutLocal(std::to_string(accessTokenIDEx), ShareOptionsUtil::GetEnumStr(shareOption)) != E_OK) {
        ZLOGE("Store get unifiedData failed, intention: %{public}d.", shareOption);
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::GetAppShareOption(const std::string &intention, int32_t &shareOption)
{
    if (intention.empty()) {
        ZLOGE("GetAppShareOption : para is invalid, %{public}s is invalid.", intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }
    std::string appShareOption;
    int32_t ret = store->GetLocal(std::to_string(accessTokenIDEx), appShareOption);
    if (ret != E_OK) {
        ZLOGE("GetAppShareOption empty, intention: %{public}s.", intention.c_str());
        return ret;
    }
    ZLOGI("GetAppShareOption, intention: %{public}s, appShareOption:%{public}s.",
          intention.c_str(), appShareOption.c_str());
    shareOption = ShareOptionsUtil::GetEnumNum(appShareOption);
    return E_OK;
}

int32_t UdmfServiceImpl::RemoveAppShareOption(const std::string &intention)
{
    if (intention.empty()) {
        ZLOGE("intention: %{public}s is invalid.", intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    if (!isSystemApp) {
        ZLOGE("no system permission, intention: %{public}s.", intention.c_str());
        return E_NO_PERMISSION;
    }
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData unifiedData;
    if (store->DeleteLocal(std::to_string(accessTokenIDEx)) != E_OK) {
        ZLOGE("Store DeleteLocal failed, intention: %{public}s.", intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::OnInitialize()
{
    ZLOGD("start");
    Status status = LifeCycleManager::GetInstance().OnStart();
    if (status != E_OK) {
        ZLOGE("OnStart execute failed, status: %{public}d", status);
    }
    status = LifeCycleManager::GetInstance().StartLifeCycleTimer();
    if (status != E_OK) {
        ZLOGE("StartLifeCycleTimer start failed, status: %{public}d", status);
    }
    return DistributedData::FeatureSystem::STUB_SUCCESS;
}

int32_t UdmfServiceImpl::QueryDataCommon(
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
    store = StoreCache::GetInstance().GetStore(intention);
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

int32_t UdmfServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    StoreCache::GetInstance().SetThreadPool(bindInfo.executors);
    LifeCycleManager::GetInstance().SetThreadPool(bindInfo.executors);
    UriPermissionManager::GetInstance().SetThreadPool(bindInfo.executors);
    return 0;
}

int32_t UdmfServiceImpl::ObtainAsynProcess(AsyncProcessInfo &processInfo)
{
    processInfo = asyncProcessInfo_;
    if (syncingData_ && processInfo.syncStatus != ASYNC_RUNNING) {
        processInfo.syncStatus = ASYNC_RUNNING;
        processInfo.srcDevName = syncingDevName_;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::ClearAsynProcess()
{
    (void)memset_s(&asyncProcessInfo_, sizeof(asyncProcessInfo_), 0, sizeof(asyncProcessInfo_));
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
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

#include "ipc_skeleton.h"
#include "tokenid_kit.h"

#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "checker_manager.h"
#include "device_manager_adapter.h"
#include "iservice_registry.h"
#include "lifecycle/lifecycle_manager.h"
#include "log_print.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "preprocess_utils.h"
#include "reporter.h"
#include "store_account_observer.h"
#include "system_ability_definition.h"
#include "uri_permission_manager.h"
#include "udmf_radar_reporter.h"
#include "unified_data_helper.h"
#include "utils/anonymous.h"

namespace OHOS {
namespace UDMF {
using namespace Security::AccessToken;
using FeatureSystem = DistributedData::FeatureSystem;
using UdmfBehaviourMsg = OHOS::DistributedDataDfx::UdmfBehaviourMsg;
using Reporter = OHOS::DistributedDataDfx::Reporter;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using namespace RadarReporter;
using namespace DistributedKv;
constexpr const char *DRAG_AUTHORIZED_PROCESSES[] = {"msdp_sa", "collaboration_service"};
constexpr const char *DATA_PREFIX = "udmf://";
constexpr const char *FILE_SCHEME = "file";
constexpr const char *PRIVILEGE_READ_AND_KEEP = "readAndKeep";
constexpr const char *MANAGE_UDMF_APP_SHARE_OPTION = "ohos.permission.MANAGE_UDMF_APP_SHARE_OPTION";
constexpr const char *DEVICE_2IN1_TAG = "2in1";
constexpr const char *DEVICE_PHONE_TAG = "phone";
constexpr const char *DEVICE_DEFAULT_TAG = "default";
constexpr const char *HAP_LIST[] = {"com.ohos.pasteboarddialog"};
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
    DistributedData::AccountDelegate::GetInstance()->Subscribe(observer);
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
    Reporter::GetInstance()->GetBehaviourReporter()->UDMFReport(msg);
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
    PreProcessUtils::SetRecordUid(unifiedData);

    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }

    if (store->Put(unifiedData) != E_OK) {
        ZLOGE("Put unified data failed:%{public}s", intention.c_str());
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
    TransferToEntriesIfNeed(query, unifiedData);
    auto errFind = ERROR_MAP.find(res);
    msg.result = errFind == ERROR_MAP.end() ? "E_ERROR" : errFind->second;
    for (const auto &record : unifiedData.GetRecords()) {
        for (const auto &type : record->GetUtdIds()) {
            types.append("-").append(type);
        }
    }
    msg.dataType = types;
    msg.dataSize = unifiedData.GetSize();
    Reporter::GetInstance()->GetBehaviourReporter()->UDMFReport(msg);
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
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    int32_t res = store->Get(query.key, unifiedData);
    if (res != E_OK) {
        ZLOGE("Get data failed,res:%{public}d,key:%{public}s", res, query.key.c_str());
        return res;
    }

    if (!unifiedData.IsComplete()) {
        ZLOGE("Get data incomplete,key:%{public}s", query.key.c_str());
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
            RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                BizScene::GET_DATA, GetDataStage::GRANT_URI_PERMISSION, StageRes::FAILED, ret);
            ZLOGE("ProcessUri failed:%{public}d", ret);
            return E_NO_PERMISSION;
        }
    }
    if (!IsReadAndKeep(runtime->privileges, query)) {
        if (LifeCycleManager::GetInstance().OnGot(key) != E_OK) {
            ZLOGE("Remove data failed:%{public}s", key.intention.c_str());
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
    std::vector<Uri> allUri;
    int32_t verifyRes = ProcessCrossDeviceData(query.tokenId, unifiedData, allUri);
    if (verifyRes != E_OK) {
        ZLOGE("verify unifieddata fail, key=%{public}s, stauts=%{public}d", query.key.c_str(), verifyRes);
        return verifyRes;
    }
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        ZLOGE("Get bundleName fail,key=%{public}s,tokenId=%d", query.key.c_str(), query.tokenId);
        return E_ERROR;
    }
    std::string sourceDeviceId = unifiedData.GetRuntime()->deviceId;
    if (localDeviceId == sourceDeviceId && query.tokenId == unifiedData.GetRuntime()->tokenId) {
        ZLOGW("No uri permissions needed,queryKey=%{public}s", query.key.c_str());
        return E_OK;
    }
    if (UriPermissionManager::GetInstance().GrantUriPermission(allUri, query.tokenId, query.key) != E_OK) {
        ZLOGE("GrantUriPermission fail, bundleName=%{public}s, key=%{public}s.",
            bundleName.c_str(), query.key.c_str());
        return E_NO_PERMISSION;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::ProcessCrossDeviceData(uint32_t tokenId, UnifiedData &unifiedData, std::vector<Uri> &uris)
{
    if (unifiedData.GetRuntime() == nullptr) {
        ZLOGE("Get runtime empty!");
        return E_DB_ERROR;
    }
    bool isLocal = PreProcessUtils::GetLocalDeviceId() == unifiedData.GetRuntime()->deviceId;
    auto records = unifiedData.GetRecords();
    bool hasError = false;
    PreProcessUtils::ProcessFileType(records, [&] (std::shared_ptr<Object> obj) {
        if (hasError) {
            return false;
        }
        std::string oriUri;
        obj->GetValue(ORI_URI, oriUri);
        if (oriUri.empty()) {
            ZLOGW("Get uri is empty.");
            return false;
        }
        Uri uri(oriUri);
        std::string scheme = uri.GetScheme();
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
        if (!isLocal) {
            std::string remoteUri;
            obj->GetValue(REMOTE_URI, remoteUri);
            if (remoteUri.empty() && scheme == FILE_SCHEME) {
                ZLOGE("Remote URI required for cross-device");
                hasError = true;
                return false;
            }
            if (!remoteUri.empty()) {
                obj->value_.insert_or_assign(ORI_URI, remoteUri); // cross dev, need dis path.
                uri = Uri(remoteUri);
                scheme = uri.GetScheme();
                std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
            }
        }
        if (uri.GetAuthority().empty() || scheme != FILE_SCHEME) {
            ZLOGW("Empty authority or scheme not file");
            return false;
        }
        uris.push_back(uri);
        return true;
    });
    PreProcessUtils::ProcessHtmlFileUris(tokenId, unifiedData, isLocal, uris);
    return hasError ? E_ERROR : E_OK;
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
        ZLOGW("DataSet empty,key:%{public}s,intention:%{public}d", query.key.c_str(), query.intention);
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
        ZLOGE("data or key is invalid,key=%{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string bundleName;
    PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName);
    if (key.bundleName != bundleName && !HasDatahubPriviledge(bundleName)) {
        ZLOGE("update data failed by %{public}s, key: %{public}s.", bundleName.c_str(), query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    UnifiedData data;
    int32_t res = store->Get(query.key, data);
    if (res != E_OK) {
        ZLOGE("Get data failed:%{public}s", key.intention.c_str());
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
    if (runtime->tokenId != query.tokenId && !HasDatahubPriviledge(bundleName)) {
        ZLOGE("Update failed: tokenId mismatch");
        return E_INVALID_PARAMETERS;
    }
    runtime->lastModifiedTime = PreProcessUtils::GetTimestamp();
    unifiedData.SetRuntime(*runtime);
    PreProcessUtils::SetRecordUid(unifiedData);
    if (store->Update(unifiedData) != E_OK) {
        ZLOGE("Unified data update failed:%{public}s", key.intention.c_str());
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
        ZLOGE("No data to delete for this application");
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
        ZLOGE("Invalid unified key:%{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }

    if (store->GetSummary(key, summary) != E_OK) {
        ZLOGE("Store get summary failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::AddPrivilege(const QueryOption &query, Privilege &privilege)
{
    ZLOGD("start");
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Invalid unified key:%{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    std::string processName;
    if (!PreProcessUtils::GetNativeProcessNameByToken(query.tokenId, processName)) {
        return E_ERROR;
    }

    if (key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        if (find(DRAG_AUTHORIZED_PROCESSES, std::end(DRAG_AUTHORIZED_PROCESSES), processName) ==
            std::end(DRAG_AUTHORIZED_PROCESSES)) {
            ZLOGE("Process:%{public}s lacks permission for intention:drag", processName.c_str());
            return E_NO_PERMISSION;
        }
    } else {
        ZLOGE("Intention: %{public}s has no authorized processes", key.intention.c_str());
        return E_NO_PERMISSION;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }

    Runtime runtime;
    auto res = store->GetRuntime(query.key, runtime);
    if (res == E_NOT_FOUND) {
        privilegeCache_[query.key] = privilege;
        ZLOGW("Add privilege in cache, key: %{public}s.", query.key.c_str());
        return E_OK;
    }
    if (res != E_OK) {
        ZLOGE("Get runtime failed, res:%{public}d, key:%{public}s.", res, query.key.c_str());
        return res;
    }
    runtime.privileges.emplace_back(privilege);
    res = store->PutRuntime(query.key, runtime);
    if (res != E_OK) {
        ZLOGE("Update runtime failed, res:%{public}d, key:%{public}s", res, query.key.c_str());
    }
    return res;
}

int32_t UdmfServiceImpl::Sync(const QueryOption &query, const std::vector<std::string> &devices)
{
    ZLOGD("start");
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::IDLE, BizState::DFX_BEGIN);
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::FAILED, E_INVALID_PARAMETERS, BizState::DFX_END);
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    RegisterAsyncProcessInfo(query.key);
    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::FAILED, E_DB_ERROR, BizState::DFX_END);
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    auto callback = [this, query](AsyncProcessInfo &syncInfo) {
        if (query.key.empty()) {
            return;
        }
        syncInfo.businessUdKey = query.key;
        std::lock_guard<std::mutex> lock(mutex_);
        asyncProcessInfoMap_.insert_or_assign(syncInfo.businessUdKey, syncInfo);
        ZLOGD("store.Sync: name=%{public}s, id=%{public}u, status=%{public}u, total=%{public}u, finish=%{public}u",
            syncInfo.srcDevName.c_str(), syncInfo.syncId, syncInfo.syncStatus,
            syncInfo.syncTotal, syncInfo.syncFinished);
    };
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::SUCCESS);
    if (store->Sync(devices, callback) != E_OK) {
        ZLOGE("Store sync failed:%{public}s", key.intention.c_str());
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, E_DB_ERROR, BizState::DFX_END);
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::IsRemoteData(const QueryOption &query, bool &result)
{
    UnifiedKey key(query.key);
    if (!key.IsValid()) {
        ZLOGE("Invalid unified key:%{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData unifiedData;
    if (store->Get(query.key, unifiedData) != E_OK) {
        ZLOGE("Store get unifiedData failed:%{public}s", key.intention.c_str());
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
        ZLOGE("para is invalid,intention:%{public}s,shareOption:%{public}d",
              intention.c_str(), shareOption);
        return E_INVALID_PARAMETERS;
    }

    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    bool hasSharePermission = VerifyPermission(MANAGE_UDMF_APP_SHARE_OPTION, IPCSkeleton::GetCallingTokenID());
    if (!isSystemApp && !hasSharePermission) {
        ZLOGE("No system or shareOption permission,intention:%{public}s", intention.c_str());
        return E_NO_PERMISSION;
    }
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }

    std::string shareOptionTmp;
    if (store->GetLocal(std::to_string(accessTokenIDEx), shareOptionTmp) == E_OK) {
        ZLOGE("SetAppShareOption failed,shareOption already set:%{public}s", shareOptionTmp.c_str());
        return E_SETTINGS_EXISTED;
    }

    if (store->PutLocal(std::to_string(accessTokenIDEx), ShareOptionsUtil::GetEnumStr(shareOption)) != E_OK) {
        ZLOGE("Store get unifiedData failed:%{public}d", shareOption);
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::GetAppShareOption(const std::string &intention, int32_t &shareOption)
{
    if (intention.empty()) {
        ZLOGE("intention is empty:%{public}s", intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    std::string appShareOption;
    int32_t ret = store->GetLocal(std::to_string(accessTokenIDEx), appShareOption);
    if (ret != E_OK) {
        ZLOGW("GetLocal failed:%{public}s", intention.c_str());
        return ret;
    }
    ZLOGI("GetLocal ok intention:%{public}s,appShareOption:%{public}s", intention.c_str(), appShareOption.c_str());
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
    bool hasSharePermission = VerifyPermission(MANAGE_UDMF_APP_SHARE_OPTION, IPCSkeleton::GetCallingTokenID());
    if (!isSystemApp && !hasSharePermission) {
        ZLOGE("No system or shareOption permission:%{public}s", intention.c_str());
        return E_NO_PERMISSION;
    }
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }

    UnifiedData unifiedData;
    if (store->DeleteLocal(std::to_string(accessTokenIDEx)) != E_OK) {
        ZLOGE("Store DeleteLocal failed:%{public}s", intention.c_str());
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
        ZLOGE("Get store failed:%{public}s", intention.c_str());
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
    return 0;
}

int32_t UdmfServiceImpl::ObtainAsynProcess(AsyncProcessInfo &processInfo)
{
    if (processInfo.businessUdKey.empty()) {
        return E_INVALID_PARAMETERS;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (asyncProcessInfoMap_.empty()) {
        processInfo.syncStatus = AsyncTaskStatus::ASYNC_SUCCESS;
        processInfo.srcDevName = "Local";
        return E_OK;
    }
    auto it = asyncProcessInfoMap_.find(processInfo.businessUdKey);
    if (it == asyncProcessInfoMap_.end()) {
        processInfo.syncStatus = AsyncTaskStatus::ASYNC_SUCCESS;
        processInfo.srcDevName = "Local";
        return E_OK;
    }
    auto asyncProcessInfo = asyncProcessInfoMap_.at(processInfo.businessUdKey);
    processInfo.syncStatus = asyncProcessInfo.syncStatus;
    processInfo.srcDevName = asyncProcessInfo.srcDevName;
    return E_OK;
}

int32_t UdmfServiceImpl::ClearAsynProcessByKey(const std::string & businessUdKey)
{
    ZLOGI("ClearAsynProcessByKey begin.");
    std::lock_guard<std::mutex> lock(mutex_);
    if (asyncProcessInfoMap_.find(businessUdKey) == asyncProcessInfoMap_.end()) {
        return E_OK;
    }
    asyncProcessInfoMap_.erase(businessUdKey);
    return E_OK;
}

int32_t UdmfServiceImpl::ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param)
{
    ZLOGI("user:%{public}s appId:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
        param.appId.c_str(), DistributedData::Anonymous::Change(param.storeId).c_str(),
        DistributedData::Anonymous::Change(identifier).c_str());

    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DmAdapter::GetInstance().GetLocalDevice().uuid });
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no meta data appId:%{public}s", param.appId.c_str());
        return E_NOT_FOUND;
    }

    for (const auto &storeMeta : metaData) {
        if (storeMeta.storeType < StoreMetaData::StoreType::STORE_KV_BEGIN ||
            storeMeta.storeType > StoreMetaData::StoreType::STORE_KV_END ||
            storeMeta.appId != DistributedData::Bootstrap::GetInstance().GetProcessLabel()) {
            continue;
        }
        auto identifierTag = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId,
                                                                                         storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }
        auto store = StoreCache::GetInstance().GetStore(storeMeta.storeId);
        if (store == nullptr) {
            ZLOGE("GetStore fail, storeId:%{public}s", DistributedData::Anonymous::Change(storeMeta.storeId).c_str());
            continue;
        }
        ZLOGI("storeId:%{public}s,appId:%{public}s,user:%{public}s",
            DistributedData::Anonymous::Change(storeMeta.storeId).c_str(),
            storeMeta.appId.c_str(), storeMeta.user.c_str());
        return E_OK;
    }
    return E_OK;
}

bool UdmfServiceImpl::VerifyPermission(const std::string &permission, uint32_t callerTokenId)
{
    if (permission.empty()) {
        return true;
    }
    int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerTokenId, permission);
    if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        ZLOGE("Permission denied. status:%{public}d, token:0x%{public}x, permission:%{public}s",
            status, callerTokenId, permission.c_str());
        return false;
    }
    return true;
}

bool UdmfServiceImpl::HasDatahubPriviledge(const std::string &bundleName)
{
    uint64_t accessTokenIDEx = IPCSkeleton::GetCallingFullTokenID();
    bool isSystemApp = TokenIdKit::IsSystemAppByFullTokenID(accessTokenIDEx);
    return std::find(std::begin(HAP_LIST), std::end(HAP_LIST), bundleName) != std::end(HAP_LIST) && isSystemApp;
}

void UdmfServiceImpl::RegisterAsyncProcessInfo(const std::string &businessUdKey)
{
    AsyncProcessInfo info;
    std::lock_guard<std::mutex> lock(mutex_);
    asyncProcessInfoMap_.insert_or_assign(businessUdKey, std::move(info));
}

int32_t UdmfServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    ZLOGI("user change, code:%{public}u, user:%{public}s, account:%{public}s", code, user.c_str(), account.c_str());
    if (code == static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_SWITCHED)) {
        StoreCache::GetInstance().CloseStores();
    }
    return Feature::OnUserChange(code, user, account);
}

void UdmfServiceImpl::TransferToEntriesIfNeed(const QueryOption &query, UnifiedData &unifiedData)
{
    if (unifiedData.IsNeedTransferToEntries() && IsNeedTransferDeviceType(query)) {
        unifiedData.TransferToEntries(unifiedData);
    }
}

bool UdmfServiceImpl::IsNeedTransferDeviceType(const QueryOption &query)
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return false;
    }
    auto bundleMgrProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrProxy == nullptr) {
        ZLOGE("Failed to Get BMS SA.");
        return false;
    }
    auto bundleManager = iface_cast<AppExecFwk::IBundleMgr>(bundleMgrProxy);
    if (bundleManager == nullptr) {
        ZLOGE("Failed to get bundle manager");
        return false;
    }
    std::string bundleName;
    PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName);
    int32_t userId = DistributedData::AccountDelegate::GetInstance()->GetUserByToken(
        IPCSkeleton::GetCallingFullTokenID());
    AppExecFwk::BundleInfo bundleInfo;
    bundleManager->GetBundleInfoV9(bundleName, static_cast<int32_t>(
        AppExecFwk::GetBundleInfoFlag::GET_BUNDLE_INFO_WITH_HAP_MODULE), bundleInfo, userId);
    for (const auto &hapModuleInfo : bundleInfo.hapModuleInfos) {
        if (std::find(hapModuleInfo.deviceTypes.begin(), hapModuleInfo.deviceTypes.end(),
            DEVICE_PHONE_TAG) == hapModuleInfo.deviceTypes.end() 
            && std::find(hapModuleInfo.deviceTypes.begin(), hapModuleInfo.deviceTypes.end(),
            DEVICE_DEFAULT_TAG) == hapModuleInfo.deviceTypes.end()
            && std::find(hapModuleInfo.deviceTypes.begin(), hapModuleInfo.deviceTypes.end(),
            DEVICE_2IN1_TAG) != hapModuleInfo.deviceTypes.end()) {
            return true;
        }
    }
    return false;
}
} // namespace UDMF
} // namespace OHOS
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
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_proxy.h"
#include "checker/checker_manager.h"
#include "checker_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "iservice_registry.h"
#include "lifecycle/lifecycle_manager.h"
#include "log_print.h"
#include "metadata/capability_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "preprocess_utils.h"
#include "dfx/reporter.h"
#include "system_ability_definition.h"
#include "uri_permission_manager.h"
#include "udmf_radar_reporter.h"
#include "udmf_utils.h"
#include "unified_data_helper.h"
#include "utils/anonymous.h"
#include "permission_validator.h"

namespace OHOS {
namespace UDMF {
using namespace Security::AccessToken;
using namespace OHOS::DistributedHardware;
using namespace OHOS::DistributedData;
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
constexpr uint32_t FOUNDATION_UID = 5523;
constexpr uint32_t WAIT_TIME = 800;
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
        RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
            BizScene::SET_DATA, SetDataStage::SET_DATA_SERVICE_BEGIN, StageRes::IDLE, bundleName);
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
    if (PreProcessUtils::FillRuntimeInfo(unifiedData, option) != E_OK) {
        ZLOGE("Imputation failed");
        return E_ERROR;
    }
    std::string intention = unifiedData.GetRuntime()->key.intention;
    if (intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        int32_t ret = PreProcessUtils::HandleFileUris(option.tokenId, unifiedData);
        if (ret != E_OK) {
            ZLOGE("HandleFileUris failed, ret: %{public}d, bundleName:%{public}s.", ret,
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
    int32_t status = store->Put(unifiedData);
    if (status != E_OK) {
        ZLOGE("Put unified data failed:%{public}s, status:%{public}d", intention.c_str(), status);
        HandleDbError(intention, status);
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
        Reporter::GetInstance()->GetBehaviourReporter()->UDMFReport(msg);
        ZLOGE("Failed to get bundle name by token, token:%{public}d", query.tokenId);
        return E_ERROR;
    }
    msg.appId = bundleName;

    bool handledByDelay = HandleDelayLoad(query, unifiedData, res);
    if (!handledByDelay) {
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
    Reporter::GetInstance()->GetBehaviourReporter()->UDMFReport(msg);
    if (res != E_OK) {
        ZLOGE("GetData failed, res:%{public}d, key:%{public}s", res, query.key.c_str());
    }
    return res;
}

bool UdmfServiceImpl::HandleDelayLoad(const QueryOption &query, UnifiedData &unifiedData, int32_t &res)
{
    return dataLoadCallback_.ComputeIfPresent(query.key, [&](const auto &key, auto &callback) {
        std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
        auto [found, cache] = blockDelayDataCache_.Find(key);
        if (!found) {
            blockData = std::make_shared<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>>(WAIT_TIME);
            blockDelayDataCache_.Insert(key, BlockDelayData{query.tokenId, blockData});
            callback->HandleDelayObserver(key, DataLoadInfo());
        } else {
            blockData = cache.blockData;
        }
        ZLOGI("Start waiting for data, key:%{public}s", key.c_str());
        auto dataOpt = blockData->GetValue();
        if (dataOpt.has_value()) {
            unifiedData = *dataOpt;
            blockDelayDataCache_.Erase(key);
            return false;
        }
        res = E_NOT_FOUND;
        return true;
    });
}

bool UdmfServiceImpl::CheckDragParams(UnifiedKey &key, const QueryOption &query)
{
    if (!key.IsValid()) {
        ZLOGE("Unified key: %{public}s is invalid.", query.key.c_str());
        return false;
    }
    if (key.intention != UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        ZLOGE("Invalid intention:%{public}s", key.intention.c_str());
        return false;
    }
    return true;
}

int32_t UdmfServiceImpl::RetrieveData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!CheckDragParams(key, query)) {
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
        HandleDbError(key.intention, res);
        return res;
    }

    if (!unifiedData.IsComplete()) {
        ZLOGE("Get data incomplete,key:%{public}s", query.key.c_str());
        return E_NOT_FOUND;
    }
    std::shared_ptr<Runtime> runtime = unifiedData.GetRuntime();
    if (runtime == nullptr) {
        return E_DB_ERROR;
    }

    res = VerifyDataAccessPermission(runtime, query, unifiedData);
    if (res != E_OK) {
        return res;
    }
    if (!IsReadAndKeep(runtime->privileges, query) && LifeCycleManager::GetInstance().OnGot(key) != E_OK) {
        ZLOGE("Remove data failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    int32_t ret = ProcessUri(query, unifiedData);
    if (ret != E_OK) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::GET_DATA, GetDataStage::GRANT_URI_PERMISSION, StageRes::FAILED, ret);
        ZLOGE("ProcessUri failed:%{public}d", ret);
        return E_NO_PERMISSION;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(cacheMutex_);
        privilegeCache_.erase(query.key);
    }
    PreProcessUtils::SetRemoteData(unifiedData);
    TransferToEntriesIfNeed(query, unifiedData);
    return E_OK;
}

int32_t UdmfServiceImpl::VerifyDataAccessPermission(std::shared_ptr<Runtime> runtime, const QueryOption &query,
    const UnifiedData &unifiedData)
{
    CheckerManager::CheckInfo info;
    info.tokenId = query.tokenId;

    if (!CheckerManager::GetInstance().IsValid(runtime->privileges, info) && !IsPermissionInCache(query)) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::GET_DATA, GetDataStage::VERIFY_PRIVILEGE, StageRes::FAILED, E_NO_PERMISSION);
        ZLOGE("Get data failed,key:%{public}s", query.key.c_str());
        return E_NO_PERMISSION;
    }

    return E_OK;
}

bool UdmfServiceImpl::IsPermissionInCache(const QueryOption &query)
{
    std::lock_guard<std::recursive_mutex> lock(cacheMutex_);
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

    std::lock_guard<std::recursive_mutex> lock(cacheMutex_);
    auto iter = privilegeCache_.find(query.key);
    if (iter != privilegeCache_.end() && iter->second.tokenId == query.tokenId &&
        iter->second.readPermission == PRIVILEGE_READ_AND_KEEP) {
        return true;
    }
    return false;
}

int32_t UdmfServiceImpl::ProcessUri(const QueryOption &query, UnifiedData &unifiedData)
{
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    int32_t verifyRes = ProcessCrossDeviceData(query.tokenId, unifiedData, readUris, writeUris);
    if (verifyRes != E_OK) {
        ZLOGE("verify unifieddata fail, key=%{public}s, stauts=%{public}d", query.key.c_str(), verifyRes);
        return verifyRes;
    }
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(query.tokenId, bundleName)) {
        ZLOGE("Get bundleName fail,key=%{public}s,tokenId=%d", query.key.c_str(), query.tokenId);
        return E_ERROR;
    }
    bool isLocal = PreProcessUtils::GetLocalDeviceId() == unifiedData.GetRuntime()->deviceId;
    if (isLocal && query.tokenId == unifiedData.GetRuntime()->tokenId) {
        ZLOGW("No uri permissions needed,queryKey=%{public}s", query.key.c_str());
        return E_OK;
    }
    if (isLocal && !VerifySavedTokenId(unifiedData.GetRuntime())) {
        ZLOGE("VerifySavedTokenId fail");
        return E_ERROR;
    }
    uint32_t sourceTokenId = unifiedData.GetRuntime()->tokenId;
    if (UriPermissionManager::GetInstance().GrantUriPermission(
        readUris, writeUris, query.tokenId, sourceTokenId, isLocal) != E_OK) {
        ZLOGE("GrantUriPermission fail, bundleName=%{public}s, key=%{public}s.",
            bundleName.c_str(), query.key.c_str());
        return E_NO_PERMISSION;
    }
    return E_OK;
}

bool UdmfServiceImpl::VerifySavedTokenId(std::shared_ptr<Runtime> runtime)
{
    uint32_t tokenId = runtime->tokenId;
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(tokenId, bundleName)) {
        ZLOGE("Get bundleName fail");
        return false;
    }
    if (bundleName != runtime->sourcePackage) {
        return false;
    }
    return true;
}

int32_t UdmfServiceImpl::ProcessCrossDeviceData(uint32_t tokenId, UnifiedData &unifiedData,
    std::vector<Uri> &readUris, std::vector<Uri> &writeUris)
{
    if (unifiedData.GetRuntime() == nullptr) {
        ZLOGE("Get runtime empty!");
        return E_DB_ERROR;
    }
    bool isLocal = PreProcessUtils::GetLocalDeviceId() == unifiedData.GetRuntime()->deviceId;
    bool hasError = false;
    PreProcessUtils::ProcessFileType(unifiedData.GetRecords(), [&] (std::shared_ptr<Object> obj) {
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
        if (uri.GetAuthority().empty() || scheme != FILE_SCHEME) {
            ZLOGW("Empty authority or scheme not file");
            return false;
        }
        if (!isLocal) {
            std::string remoteUri;
            obj->GetValue(REMOTE_URI, remoteUri);
            if (remoteUri.empty()) {
                ZLOGE("Remote URI required for cross-device");
                hasError = true;
                return false;
            }
            uri = Uri(remoteUri);
            obj->value_.insert_or_assign(ORI_URI, std::move(remoteUri)); // cross dev, need dis path.
            scheme = uri.GetScheme();
            std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
            if (uri.GetAuthority().empty() || scheme != FILE_SCHEME) {
                ZLOGW("Empty authority or scheme not file");
                return false;
            }
        }
        int32_t permission;
        if (obj->GetValue(PERMISSION_POLICY, permission)) {
            permission == PermissionPolicy::READ_WRITE ? writeUris.emplace_back(uri) : readUris.emplace_back(uri);
        }
        return true;
    });
    PreProcessUtils::ProcessHtmlFileUris(tokenId, unifiedData, isLocal, readUris, writeUris);
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
        if (query.intention == Intention::UD_INTENTION_DATA_HUB &&
            data.GetRuntime()->visibility == VISIBILITY_OWN_PROCESS &&
            query.tokenId != data.GetRuntime()->tokenId) {
            continue;
        } else {
            unifiedDataSet.push_back(std::move(data));
        }
    }
    if (!IsFileMangerSa() && ProcessData(query, unifiedDataSet) != E_OK) {
        ZLOGE("Query no permission.");
        return E_NO_PERMISSION;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::UpdateData(const QueryOption &query, UnifiedData &unifiedData)
{
    UnifiedKey key(query.key);
    if (!IsValidInput(query, unifiedData, key)) {
        return E_INVALID_PARAMETERS;
    }
    std::string bundleName;
    std::string specificBundleName;
    if (!PreProcessUtils::GetSpecificBundleNameByTokenId(query.tokenId, specificBundleName, bundleName)) {
        ZLOGE("GetSpecificBundleNameByTokenId failed, tokenid:%{public}u", query.tokenId);
        return E_ERROR;
    }
    if (key.bundleName != specificBundleName && !HasDatahubPriviledge(bundleName)) {
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
        HandleDbError(key.intention, res);
        return res;
    }
    auto verifyRes = VerifyUpdatePermission(query, data, bundleName);
    if (verifyRes != E_OK) {
        ZLOGE("VerifyUpdatePermission failed:%{public}d, key: %{public}s.", verifyRes, query.key.c_str());
        return verifyRes;
    }
    std::shared_ptr<Runtime> runtime = data.GetRuntime();
    runtime->lastModifiedTime = PreProcessUtils::GetTimestamp();
    unifiedData.SetRuntime(*runtime);
    PreProcessUtils::SetRecordUid(unifiedData);
    if ((res = store->Update(unifiedData)) != E_OK) {
        ZLOGE("Unified data update failed:%{public}s", key.intention.c_str());
        HandleDbError(key.intention, res);
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::VerifyUpdatePermission(const QueryOption &query, UnifiedData &data, std::string &bundleName)
{
    if (data.IsEmpty()) {
        ZLOGE("Invalid parameter, unified data has no record");
        return E_INVALID_PARAMETERS;
    }
    std::shared_ptr<Runtime> runtime = data.GetRuntime();
    if (runtime == nullptr) {
        ZLOGE("Invalid parameter, runtime is nullptr.");
        return E_DB_ERROR;
    }
    if (runtime->tokenId != query.tokenId && !HasDatahubPriviledge(bundleName) &&
        CheckAppId(runtime, bundleName) != E_OK) {
        ZLOGE("Update failed: tokenId or appId mismatch, bundleName: %{public}s", bundleName.c_str());
        return E_INVALID_PARAMETERS;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::CheckAppId(std::shared_ptr<Runtime> runtime, const std::string &bundleName)
{
    if (runtime->appId.empty()) {
        ZLOGE("Update failed: Invalid parameter, runtime->appId is empty");
        return E_INVALID_PARAMETERS;
    }
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(
        { IPCSkeleton::GetCallingUid(), runtime->tokenId, bundleName });
    if (appId.empty() || appId != runtime->appId) {
        ZLOGE("Update failed: runtime->appId %{public}s and bundleName appId %{public}s mismatch",
            runtime->appId.c_str(), appId.c_str());
        return E_INVALID_PARAMETERS;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::DeleteData(const QueryOption &query, std::vector<UnifiedData> &unifiedDataSet)
{
    ZLOGD("start");
    UnifiedKey key(query.key);
    if (!key.IsValid() && !key.key.empty()) {
        ZLOGE("invalid key, query.key: %{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string intention = FindIntentionMap(query.intention);
    if (!IsValidOptionsNonDrag(key, intention)) {
        ZLOGE("invalid option, query.key: %{public}s, intention: %{public}d", query.key.c_str(), query.intention);
        return E_INVALID_PARAMETERS;
    }
    std::vector<UnifiedData> dataSet;
    std::shared_ptr<Store> store;
    int32_t status = QueryDataCommon(query, dataSet, store);
    if (status != E_OK) {
        ZLOGE("QueryDataCommon failed.");
        return status;
    }
    if (dataSet.empty()) {
        ZLOGW("DataSet has no data, key: %{public}s, intention: %{public}d.", query.key.c_str(), query.intention);
        return E_OK;
    }
    std::shared_ptr<Runtime> runtime;
    std::string appId;
    std::vector<std::string> deleteKeys;
    for (const auto &data : dataSet) {
        auto runtime = data.GetRuntime();
        if (!CheckDeleteDataPermission(appId, runtime, query)) {
            continue;
        }
        unifiedDataSet.push_back(data);
        deleteKeys.emplace_back(UnifiedKey(runtime->key.key).GetKeyCommonPrefix());
    }
    if (deleteKeys.empty()) {
        ZLOGE("No data to delete for this application");
        return E_OK;
    }
    ZLOGI("Delete data start. size: %{public}zu.", deleteKeys.size());
    status = store->DeleteBatch(deleteKeys);
    if (status != E_OK) {
        ZLOGE("Remove data failed.");
        HandleDbError(key.intention, status);
        return E_DB_ERROR;
    }
    return E_OK;
}

bool UdmfServiceImpl::CheckDeleteDataPermission(std::string &appId, const std::shared_ptr<Runtime> &runtime,
    const QueryOption &query)
{
    if (runtime == nullptr) {
        ZLOGE("Invalid runtime.");
        return false;
    }
    if (runtime->tokenId == query.tokenId) {
        return true;
    }
    std::string bundleName;
    std::string specificBundleName;
    if (!PreProcessUtils::GetSpecificBundleNameByTokenId(query.tokenId, specificBundleName, bundleName)) {
        ZLOGE("GetSpecificBundleNameByTokenId failed, tokenid:%{public}u", query.tokenId);
        return false;
    }
    if (CheckAppId(runtime, bundleName) != E_OK) {
        ZLOGE("Delete failed: tokenId or appId mismatch, bundleName: %{public}s", bundleName.c_str());
        return false;
    }
    return true;
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
    int32_t status = store->GetSummary(key, summary);
    if (status != E_OK) {
        ZLOGE("Store get summary failed:%{public}s", key.intention.c_str());
        HandleDbError(key.intention, status);
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
        ZLOGE("GetNativeProcessNameByToken is faild");
        return E_ERROR;
    }

    if (CheckAddPrivilegePermission(key, processName, query) != E_OK) {
        ZLOGE("Intention:%{public}s no permission", key.intention.c_str());
        return E_NO_PERMISSION;
    }
    if (!UTILS::IsNativeCallingToken()) {
        ZLOGE("Calling Token is not native");
        return E_NO_PERMISSION;
    }

    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }

    Runtime runtime;
    int32_t res = store->GetRuntime(query.key, runtime);
    if (res == E_NOT_FOUND) {
        std::lock_guard<std::recursive_mutex> lock(cacheMutex_);
        privilegeCache_[query.key] = privilege;
        ZLOGW("Add privilege in cache, key: %{public}s.", query.key.c_str());
        return E_OK;
    }
    if (res != E_OK) {
        ZLOGE("Get runtime failed, res:%{public}d, key:%{public}s.", res, query.key.c_str());
        HandleDbError(key.intention, res);
        return res;
    }
    runtime.privileges.emplace_back(privilege);
    res = store->PutRuntime(query.key, runtime);
    if (res != E_OK) {
        ZLOGE("Update runtime failed, res:%{public}d, key:%{public}s", res, query.key.c_str());
        HandleDbError(key.intention, res);
    }
    return res;
}

int32_t UdmfServiceImpl::Sync(const QueryOption &query, const std::vector<std::string> &devices)
{
    if (!UTILS::IsTokenNative(query.tokenId) ||
        !DistributedKv::PermissionValidator::GetInstance().CheckSyncPermission(query.tokenId)) {
        ZLOGE("Tokenid permission verification failed!");
        return E_NO_PERMISSION;
    }
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
    return StoreSync(key, query, devices);
}

int32_t UdmfServiceImpl::StoreSync(const UnifiedKey &key, const QueryOption &query,
    const std::vector<std::string> &devices)
{
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
    };
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SYNC_DATA, SyncDataStage::SYNC_BEGIN, StageRes::SUCCESS);
    int userId = 0;
    if (!AccountDelegate::GetInstance()->QueryForegroundUserId(userId)) {
        ZLOGE("QueryForegroundUserId failed");
        return E_ERROR;
    }
    auto meta = BuildMeta(key.intention, userId);
    auto uuids = DmAdapter::GetInstance().ToUUID(devices);
    if (IsNeedMetaSync(meta, uuids)) {
        bool res = MetaDataManager::GetInstance().Sync(uuids, [this, devices, callback, store] (auto &results) {
            auto successRes = ProcessResult(results);
            if (store->Sync(successRes, callback) != E_OK) {
                ZLOGE("Store sync failed");
                RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                    BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, E_DB_ERROR, BizState::DFX_END);
            }
        });
        if (!res) {
            ZLOGE("Meta sync failed");
            RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, E_DB_ERROR, BizState::DFX_END);
            return E_DB_ERROR;
        }
        return E_OK;
    }
    if (store->Sync(devices, callback) != E_OK) {
        ZLOGE("Store sync failed");
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, E_DB_ERROR, BizState::DFX_END);
        return UDMF::E_DB_ERROR;
    }
    return E_OK;
}

bool UdmfServiceImpl::IsNeedMetaSync(const StoreMetaData &meta, const std::vector<std::string> &uuids)
{
    using namespace OHOS::DistributedData;
    bool isAfterMeta = false;
    for (const auto &uuid : uuids) {
        auto metaData = meta;
        metaData.deviceId = uuid;
        CapMetaData capMeta;
        auto capKey = CapMetaRow::GetKeyFor(uuid);
        if (!MetaDataManager::GetInstance().LoadMeta(std::string(capKey.begin(), capKey.end()), capMeta) ||
            !MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData)) {
            isAfterMeta = true;
            break;
        }
        auto [exist, mask] = DeviceMatrix::GetInstance().GetRemoteMask(uuid);
        if ((mask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
        auto [existLocal, localMask] = DeviceMatrix::GetInstance().GetMask(uuid);
        if ((localMask & DeviceMatrix::META_STORE_MASK) == DeviceMatrix::META_STORE_MASK) {
            isAfterMeta = true;
            break;
        }
    }
    return isAfterMeta;
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

    Runtime runtime;
    int32_t res = store->GetRuntime(query.key, runtime);
    if (res != E_OK) {
        ZLOGE("Get runtime failed, res:%{public}d, key:%{public}s.", res, query.key.c_str());
        HandleDbError(key.intention, res);
        return E_DB_ERROR;
    }

    std::string localDeviceId = PreProcessUtils::GetLocalDeviceId();
    if (localDeviceId != runtime.deviceId) {
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
    int32_t status = store->PutLocal(std::to_string(accessTokenIDEx), ShareOptionsUtil::GetEnumStr(shareOption));
    if (status != E_OK) {
        ZLOGE("Store get unifiedData failed:%{public}d", status);
        HandleDbError(intention, status);
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
        HandleDbError(intention, ret);
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

    int32_t status = store->DeleteLocal(std::to_string(accessTokenIDEx));
    if (status != E_OK) {
        ZLOGE("Store DeleteLocal failed:%{public}s, status:%{public}d", intention.c_str(), status);
        HandleDbError(intention, status);
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
    UnifiedKey key(query.key);
    if (!key.IsValid() && !key.key.empty()) {
        ZLOGE("invalid key, query.key: %{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string intention = FindIntentionMap(query.intention);
    if (!IsValidOptionsNonDrag(key, intention)) {
        ZLOGE("Unified key: %{public}s and intention: %{public}s is invalid.", query.key.c_str(), intention.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string dataPrefix;
    if (key.key.empty()) {
        dataPrefix = DATA_PREFIX + intention;
    } else {
        dataPrefix = UnifiedKey(key.key).GetKeyCommonPrefix();
        intention = key.intention;
    }
    ZLOGD("dataPrefix = %{public}s, intention: %{public}s.", dataPrefix.c_str(), intention.c_str());
    store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    int32_t status = store->GetBatchData(dataPrefix, dataSet);
    if (status != E_OK) {
        ZLOGE("Get dataSet failed, dataPrefix: %{public}s, status:%{public}d.", dataPrefix.c_str(), status);
        HandleDbError(intention, status);
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
    ZLOGI("user:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
        DistributedData::Anonymous::Change(param.storeId).c_str(),
        DistributedData::Anonymous::Change(identifier).c_str());

    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DmAdapter::GetInstance().GetLocalDevice().uuid });
    if (!DistributedData::MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no meta data");
        return E_NOT_FOUND;
    }

    for (const auto &storeMeta : metaData) {
        if (storeMeta.storeType < StoreMetaData::StoreType::STORE_KV_BEGIN ||
            storeMeta.storeType > StoreMetaData::StoreType::STORE_KV_END ||
            storeMeta.appId != Bootstrap::GetInstance().GetProcessLabel()) {
            continue;
        }
        auto identifierTag = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId,
                                                                                         storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }
        auto store = StoreCache::GetInstance().GetStore(storeMeta.storeId);
        if (store == nullptr) {
            ZLOGE("GetStore fail, storeId:%{public}s", Anonymous::Change(storeMeta.storeId).c_str());
            continue;
        }
        ZLOGI("storeId:%{public}s,user:%{public}s", Anonymous::Change(storeMeta.storeId).c_str(),
            storeMeta.user.c_str());
        return E_OK;
    }
    return E_OK;
}

bool UdmfServiceImpl::VerifyPermission(const std::string &permission, uint32_t callerTokenId)
{
    if (permission.empty()) {
        ZLOGE("VerifyPermission failed, Permission is empty.");
        return false;
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
    ZLOGI("user change, code:%{public}u, user:%{public}s", code, user.c_str());
    if (code == static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_STOPPING)
        || code == static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_STOPPED)
        || code == static_cast<uint32_t>(DistributedData::AccountStatus::DEVICE_ACCOUNT_SWITCHED)) {
        StoreCache::GetInstance().CloseStores();
    }
    return Feature::OnUserChange(code, user, account);
}

void UdmfServiceImpl::TransferToEntriesIfNeed(const QueryOption &query, UnifiedData &unifiedData)
{
    if (unifiedData.IsNeedTransferToEntries() && IsNeedTransferDeviceType(query)) {
        unifiedData.ConvertRecordsToEntries();
    }
}

bool UdmfServiceImpl::IsNeedTransferDeviceType(const QueryOption &query)
{
    auto deviceInfo = DmAdapter::GetInstance().GetLocalDevice();
    if (deviceInfo.deviceType != DEVICE_TYPE_PC && deviceInfo.deviceType != DEVICE_TYPE_PAD
        && deviceInfo.deviceType != DEVICE_TYPE_2IN1) {
        return false;
    }
    auto bundleManager = PreProcessUtils::GetBundleMgr();
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

int32_t UdmfServiceImpl::CheckAddPrivilegePermission(UnifiedKey &key,
    std::string &processName, const QueryOption &query)
{
    if (IsFileMangerIntention(key.intention)) {
        if (IsFileMangerSa()) {
            return E_OK;
        }
        std::string intention = FindIntentionMap(query.intention);
        if (!intention.empty() && key.intention != intention) {
            ZLOGE("Query.intention no not equal to key.intention");
            return E_INVALID_PARAMETERS;
        }
        return E_OK;
    }
    if (key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DRAG)) {
        if (find(DRAG_AUTHORIZED_PROCESSES, std::end(DRAG_AUTHORIZED_PROCESSES), processName) ==
            std::end(DRAG_AUTHORIZED_PROCESSES)) {
            ZLOGE("Process:%{public}s lacks permission for intention:drag", processName.c_str());
            return E_NO_PERMISSION;
        }
        return E_OK;
    }
    ZLOGE("Check addPrivilege permission is faild.");
    return E_NO_PERMISSION;
}

bool UdmfServiceImpl::IsFileMangerSa()
{
    if (IPCSkeleton::GetCallingUid() == FOUNDATION_UID) {
        return true;
    }
    ZLOGE("Caller no permission");
    return false;
}

int32_t UdmfServiceImpl::ProcessData(const QueryOption &query, std::vector<UnifiedData> &dataSet)
{
    UnifiedKey key(query.key);
    if (!key.key.empty() && !key.IsValid()) {
        ZLOGE("invalid key, query.key: %{public}s", query.key.c_str());
        return E_INVALID_PARAMETERS;
    }
    std::string intention = FindIntentionMap(query.intention);
    if (intention == UD_INTENTION_MAP.at(UD_INTENTION_DATA_HUB) ||
        key.intention == UD_INTENTION_MAP.at(UD_INTENTION_DATA_HUB)) {
        return E_OK;
    }
    CheckerManager::CheckInfo info;
    info.tokenId = query.tokenId;
    for (auto &data : dataSet) {
        auto ret = VerifyIntentionPermission(query, data, key, info);
        if (ret != E_OK) {
            ZLOGE("Verify intention permission is faild:%{public}d", ret);
            return ret;
        }
    }
    return E_OK;
}

int32_t UdmfServiceImpl::VerifyIntentionPermission(const QueryOption &query,
    UnifiedData &data, UnifiedKey &key, CheckerManager::CheckInfo &info)
{
    std::shared_ptr<Runtime> runtime = data.GetRuntime();
    if (runtime == nullptr) {
        ZLOGE("runtime is nullptr.");
        return E_DB_ERROR;
    }
    if (!CheckerManager::GetInstance().IsValid(runtime->privileges, info)) {
        ZLOGE("Query no permission.");
        return E_NO_PERMISSION;
    }
    if (!IsReadAndKeep(runtime->privileges, query)) {
        if (LifeCycleManager::GetInstance().OnGot(key) != E_OK) {
            ZLOGE("Remove data failed:%{public}s", key.intention.c_str());
            return E_DB_ERROR;
        }
    }
    return E_OK;
}

bool UdmfServiceImpl::IsFileMangerIntention(const std::string &intention)
{
    Intention optionIntention = UnifiedDataUtils::GetIntentionByString(intention);
    if (optionIntention == UD_INTENTION_SYSTEM_SHARE ||
        optionIntention == UD_INTENTION_MENU ||
        optionIntention == UD_INTENTION_PICKER) {
        return true;
    }
    return false;
}

std::string UdmfServiceImpl::FindIntentionMap(const Intention &queryIntention)
{
    auto find = UD_INTENTION_MAP.find(queryIntention);
    return find == UD_INTENTION_MAP.end() ? "" : find->second;
}

bool UdmfServiceImpl::IsValidOptionsNonDrag(UnifiedKey &key, const std::string &intention)
{
    if (UnifiedDataUtils::IsValidOptions(key, intention)) {
        return !key.key.empty() || intention == UD_INTENTION_MAP.at(Intention::UD_INTENTION_DATA_HUB);
    }
    return false;
}

int32_t UdmfServiceImpl::SetDelayInfo(const DataLoadInfo &dataLoadInfo, sptr<IRemoteObject> iUdmfNotifier,
    std::string &key)
{
    std::string bundleName;
    std::string specificBundleName;
    auto tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    if (!PreProcessUtils::GetSpecificBundleNameByTokenId(tokenId, specificBundleName, bundleName)) {
        ZLOGE("GetSpecificBundleNameByTokenId failed, tokenid:%{public}u", tokenId);
        return E_ERROR;
    }
    UnifiedKey udkey(UD_INTENTION_MAP.at(UD_INTENTION_DRAG), specificBundleName, dataLoadInfo.sequenceKey);
    key = udkey.GetUnifiedKey();
    dataLoadCallback_.Insert(key, iface_cast<UdmfNotifierProxy>(iUdmfNotifier));

    auto store = StoreCache::GetInstance().GetStore(UD_INTENTION_MAP.at(UD_INTENTION_DRAG));
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.c_str());
        return E_DB_ERROR;
    }

    Summary summary;
    UnifiedDataHelper::GetSummaryFromLoadInfo(dataLoadInfo, summary);
    int32_t status = store->PutSummary(udkey, summary);
    if (status != E_OK) {
        ZLOGE("Put summary failed:%{public}s, status:%{public}d", key.c_str(), status);
        HandleDbError(UD_INTENTION_MAP.at(UD_INTENTION_DRAG), status);
        return E_DB_ERROR;
    }
    return E_OK;
}

int32_t UdmfServiceImpl::PushDelayData(const std::string &key, UnifiedData &unifiedData)
{
    auto delayIt = delayDataCallback_.Find(key);
    auto blockIt = blockDelayDataCache_.Find(key);
    bool isDataLoading = (delayIt.first);
    if (!isDataLoading && !blockIt.first) {
        ZLOGE("DelayData callback and block cache not exist, key:%{public}s", key.c_str());
        return E_ERROR;
    }

    CustomOption option;
    option.intention = UD_INTENTION_DRAG;
    option.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    if (PreProcessUtils::FillRuntimeInfo(unifiedData, option) != E_OK) {
        ZLOGE("Imputation failed");
        return E_ERROR;
    }
    int32_t ret = PreProcessUtils::HandleFileUris(option.tokenId, unifiedData);
    if (ret != E_OK) {
        ZLOGE("HandleFileUris failed, ret:%{public}d, key:%{public}s.", ret, key.c_str());
        return ret;
    }

    QueryOption query;
    query.tokenId = isDataLoading ? delayIt.second.tokenId : blockIt.second.tokenId;
    query.key = key;
    if (option.tokenId != query.tokenId && !IsPermissionInCache(query)) {
        ZLOGE("No permission");
        return E_NO_PERMISSION;
    }
    ret = ProcessUri(query, unifiedData);
    if (ret != E_OK) {
        ZLOGE("ProcessUri failed:%{public}d", ret);
        return E_NO_PERMISSION;
    }
    {
        std::lock_guard<std::recursive_mutex> lock(cacheMutex_);
        privilegeCache_.erase(key);
    }
    PreProcessUtils::SetRemoteData(unifiedData);
    TransferToEntriesIfNeed(query, unifiedData);

    if (!isDataLoading) {
        blockIt.second.blockData->SetValue(unifiedData);
        return E_OK;
    }
    return HandleDelayDataCallback(delayIt.second, unifiedData, key);
}

int32_t UdmfServiceImpl::HandleDelayDataCallback(DelayGetDataInfo &delayGetDataInfo, UnifiedData &unifiedData,
    const std::string &key)
{
    auto callback = iface_cast<DelayDataCallbackProxy>(delayGetDataInfo.dataCallback);
    if (callback == nullptr) {
        ZLOGE("Delay data callback is null, key:%{public}s", key.c_str());
        return E_ERROR;
    }
    callback->DelayDataCallback(key, unifiedData);
    delayDataCallback_.Erase(key);
    return E_OK;
}

int32_t UdmfServiceImpl::GetDataIfAvailable(const std::string &key, const DataLoadInfo &dataLoadInfo,
    sptr<IRemoteObject> iUdmfNotifier, std::shared_ptr<UnifiedData> unifiedData)
{
    ZLOGD("start");
    QueryOption query;
    query.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    query.key = key;
    if (unifiedData == nullptr) {
        ZLOGE("Data is null, key:%{public}s", key.c_str());
        return E_ERROR;
    }
    auto status = RetrieveData(query, *unifiedData);
    if (status == E_OK) {
        return E_OK;
    }
    if (status != E_NOT_FOUND) {
        ZLOGE("Retrieve data failed, key:%{public}s", key.c_str());
        return status;
    }
    DelayGetDataInfo delayGetDataInfo;
    delayGetDataInfo.dataCallback = iUdmfNotifier;
    delayGetDataInfo.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    delayDataCallback_.InsertOrAssign(key, std::move(delayGetDataInfo));

    auto it = dataLoadCallback_.Find(key);
    if (!it.first) {
        ZLOGE("DataLoad callback no exist, key:%{public}s", key.c_str());
        return E_ERROR;
    }
    it.second->HandleDelayObserver(key, dataLoadInfo);
    dataLoadCallback_.Erase(key);
    return E_OK;
}

bool UdmfServiceImpl::IsValidInput(const QueryOption &query, UnifiedData &unifiedData, UnifiedKey &key)
{
    if (!unifiedData.IsValid() || !key.IsValid()) {
        ZLOGE("Data or key is invalid, key = %{public}s", query.key.c_str());
        return false;
    }
    std::string intention = FindIntentionMap(query.intention);
    if (!IsValidOptionsNonDrag(key, intention) || key.intention != UD_INTENTION_MAP.at(UD_INTENTION_DATA_HUB)) {
        ZLOGE("Invalid params: key.intention = %{public}s, intention = %{public}s",
            key.intention.c_str(), intention.c_str());
        return false;
    }
    return true;
}

void UdmfServiceImpl::CloseStoreWhenCorrupted(const std::string &intention, int32_t status)
{
    if (status == E_DB_CORRUPTED) {
        ZLOGE("Kv database corrupted, start to remove store");
        StoreCache::GetInstance().RemoveStore(intention);
    }
}

void UdmfServiceImpl::HandleDbError(const std::string &intention, int32_t &status)
{
    CloseStoreWhenCorrupted(intention, status);
    if (status == E_DB_CORRUPTED) {
        // reset status to E_DB_ERROR
        status = E_DB_ERROR;
    }
}

StoreMetaData UdmfServiceImpl::BuildMeta(const std::string &storeId, int userId)
{
    StoreMetaData meta;
    meta.user = std::to_string(userId);
    meta.storeId = storeId;
    meta.bundleName = Bootstrap::GetInstance().GetProcessLabel();
    return meta;
}

std::vector<std::string> UdmfServiceImpl::ProcessResult(const std::map<std::string, int32_t> &results)
{
    std::vector<std::string> devices;
    for (const auto &[uuid, status] : results) {
        if (static_cast<DistributedDB::DBStatus>(status) == DistributedDB::DBStatus::OK) {
            DeviceMatrix::GetInstance().OnExchanged(uuid, DeviceMatrix::META_STORE_MASK);
            devices.emplace_back(uuid);
        }
    }
    ZLOGI("Meta sync finish, total size:%{public}zu, success size:%{public}zu", results.size(), devices.size());
    return devices;
}
} // namespace UDMF
} // namespace OHOS
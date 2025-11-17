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
#define LOG_TAG "PreProcessUtils"

#include "preprocess_utils.h"

#include <sstream>

#include "bundle_info.h"
#include "dds_trace.h"
#include "udmf_radar_reporter.h"
#include "accesstoken_kit.h"
#include "checker/checker_manager.h"
#include "device_manager_adapter.h"
#include "file_mount_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "udmf_radar_reporter.h"
#include "udmf_utils.h"
#include "unified_html_record_process.h"
#include "utils/crypto.h"
#include "uri_permission_manager_client.h"
#include "ipc_skeleton.h"
namespace OHOS {
namespace UDMF {
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;
static constexpr int MAXIMUM = 121;
constexpr char SPECIAL = '^';
constexpr const char *FILE_SCHEME = "file";
constexpr const char *TAG = "PreProcessUtils::";
constexpr const char *FILE_SCHEME_PREFIX = "file://";
constexpr const char *DOCS_LOCAL_TAG = "/docs/";
static constexpr uint32_t DOCS_LOCAL_PATH_SUBSTR_START_INDEX = 1;
static constexpr uint32_t VERIFY_URI_PERMISSION_MAX_SIZE = 500;
constexpr const char *TEMP_UNIFIED_DATA_FLAG = "temp_udmf_file_flag";
static constexpr size_t TEMP_UDATA_RECORD_SIZE = 1;
static constexpr uint32_t PREFIX_LEN = 24;
static constexpr uint32_t INDEX_LEN = 8;
static constexpr const char PLACE_HOLDER = '0';
using namespace OHOS::DistributedDataDfx;
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
using namespace RadarReporter;

int32_t PreProcessUtils::FillRuntimeInfo(UnifiedData &data, CustomOption &option)
{
    auto it = UD_INTENTION_MAP.find(option.intention);
    if (it == UD_INTENTION_MAP.end()) {
        return E_ERROR;
    }
    std::string bundleName;
    std::string specificBundleName;
    if (!GetSpecificBundleNameByTokenId(option.tokenId, specificBundleName, bundleName)) {
        ZLOGE("GetSpecificBundleNameByTokenId failed, tokenid:%{public}u", option.tokenId);
        return E_ERROR;
    }
    std::string intention = it->second;
    UnifiedKey key(intention, specificBundleName, GenerateId());
    Privilege privilege;
    privilege.tokenId = option.tokenId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(
        { IPCSkeleton::GetCallingUid(), option.tokenId, bundleName });
    Runtime runtime;
    runtime.key = key;
    runtime.privileges.emplace_back(privilege);
    runtime.createTime = GetTimestamp();
    runtime.sourcePackage = bundleName;
    runtime.createPackage = bundleName;
    runtime.deviceId = GetLocalDeviceId();
    runtime.recordTotalNum = static_cast<uint32_t>(data.GetRecords().size());
    runtime.tokenId = option.tokenId;
    runtime.sdkVersion = GetSdkVersionByToken(option.tokenId);
    runtime.visibility = option.visibility;
    runtime.appId = appId;
    data.SetRuntime(runtime);
    return E_OK;
}

int32_t PreProcessUtils::FillDelayRuntimeInfo(UnifiedData &data, CustomOption &option, const DataLoadInfo &info)
{
    std::string bundleName;
    std::string specificBundleName;
    if (!GetSpecificBundleNameByTokenId(option.tokenId, specificBundleName, bundleName)) {
        ZLOGE("GetSpecificBundleNameByTokenId failed, tokenid:%{public}u", option.tokenId);
        return E_ERROR;
    }
    UnifiedKey key(UD_INTENTION_MAP.at(UD_INTENTION_DRAG), specificBundleName, info.sequenceKey);
    Privilege privilege;
    privilege.tokenId = option.tokenId;
    std::string appId = DistributedData::CheckerManager::GetInstance().GetAppId(
        { IPCSkeleton::GetCallingUid(), option.tokenId, bundleName });
    Runtime runtime;
    runtime.key = key;
    runtime.privileges.emplace_back(privilege);
    runtime.createTime = GetTimestamp();
    runtime.sourcePackage = bundleName;
    runtime.createPackage = bundleName;
    runtime.deviceId = GetLocalDeviceId();
    runtime.recordTotalNum = info.recordCount;
    runtime.tokenId = option.tokenId;
    runtime.sdkVersion = GetSdkVersionByToken(option.tokenId);
    runtime.visibility = option.visibility;
    runtime.appId = appId;
    runtime.dataStatus = DataStatus::WAITING;
    data.SetRuntime(runtime);
    return E_OK;
}

std::string PreProcessUtils::GenerateId()
{
    std::vector<uint8_t> randomDevices = DistributedData::Crypto::Random(ID_LEN, MINIMUM, MAXIMUM);
    std::stringstream idStr;
    for (auto &randomDevice : randomDevices) {
        auto asc = randomDevice;
        asc = asc >= SPECIAL ? asc + 1 : asc;
        idStr << static_cast<uint8_t>(asc);
    }
    return idStr.str();
}

time_t PreProcessUtils::GetTimestamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    time_t timestamp = tp.time_since_epoch().count();
    return timestamp;
}

int32_t PreProcessUtils::GetHapUidByToken(uint32_t tokenId, int &userId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        ZLOGE("TokenType is not TOKEN_HAP");
        return E_ERROR;
    }
    Security::AccessToken::HapTokenInfo tokenInfo;
    auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result != Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        ZLOGE("GetHapUidByToken failed, result = %{public}d.", result);
        return E_ERROR;
    }
    userId = tokenInfo.userID;
    return E_OK;
}

bool PreProcessUtils::GetHapBundleNameByToken(uint32_t tokenId, std::string &bundleName)
{
    if (UTILS::IsTokenNative(tokenId)) {
        ZLOGD("TypeATokenTypeEnum is TOKEN_HAP");
        std::string processName;
        if (GetNativeProcessNameByToken(tokenId, processName)) {
            bundleName = processName;
            return true;
        }
        return false;
    }
    Security::AccessToken::HapTokenInfo hapInfo;
    if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo)
        == Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        bundleName = hapInfo.bundleName;
        return true;
    }
    ZLOGE("Get bundle name faild");
    return false;
}

bool PreProcessUtils::GetNativeProcessNameByToken(uint32_t tokenId, std::string &processName)
{
    Security::AccessToken::NativeTokenInfo nativeInfo;
    if (Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(tokenId, nativeInfo)
        != Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        return false;
    }
    processName = nativeInfo.processName;
    return true;
}

std::string PreProcessUtils::GetLocalDeviceId()
{
    auto info = DistributedData::DeviceManagerAdapter::GetInstance().GetLocalDevice();
    std::string encryptedUuid = DistributedData::DeviceManagerAdapter::GetInstance().CalcClientUuid(" ", info.uuid);
    return encryptedUuid;
}

void PreProcessUtils::SetRemoteData(UnifiedData &data)
{
    if (data.IsEmpty()) {
        ZLOGD("invalid data.");
        return;
    }
    std::shared_ptr<Runtime> runtime = data.GetRuntime();
    if (runtime->deviceId == GetLocalDeviceId()) {
        ZLOGD("not remote data.");
        return;
    }
    ZLOGD("is remote data.");
    auto records = data.GetRecords();
    ProcessFileType(records, [] (std::shared_ptr<Object> obj) {
        std::shared_ptr<Object> detailObj;
        obj->GetValue(DETAILS, detailObj);
        if (detailObj == nullptr) {
            ZLOGE("No details for object");
            return false;
        }
        UDDetails details = ObjectUtils::ConvertToUDDetails(detailObj);
        details.insert({ "isRemote", "true" });
        obj->value_[DETAILS] = ObjectUtils::ConvertToObject(details);
        return true;
    });
}

int32_t PreProcessUtils::HandleFileUris(uint32_t tokenId, UnifiedData &data)
{
    std::vector<std::string> uris;
    ProcessFileType(data.GetRecords(), [&uris](std::shared_ptr<Object> obj) {
        obj->value_[REMOTE_URI] = ""; // To ensure remoteUri is empty when save data!
        obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(NO_PERMISSION);
        std::string oriUri;
        obj->GetValue(ORI_URI, oriUri);
        if (oriUri.empty()) {
            ZLOGW("URI is empty, please check");
            return false;
        }
        Uri uri(oriUri);
        std::string scheme = uri.GetScheme();
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
        if (uri.GetAuthority().empty() || scheme != FILE_SCHEME) {
            ZLOGW("Empty URI authority or scheme not file");
            return false;
        }
        uris.push_back(oriUri);
        return true;
    });
    std::map<std::string, int32_t> htmlUris;
    GetHtmlFileUris(tokenId, data, true, htmlUris);
    for (const auto &item : htmlUris) {
        if (std::find(uris.begin(), uris.end(), item.first) == uris.end()) {
            uris.emplace_back(item.first);
        }
    }
    return ReadCheckUri(tokenId, data, uris);
}

int32_t PreProcessUtils::ReadCheckUri(uint32_t tokenId, UnifiedData &data, std::vector<std::string> &uris)
{
    if (!uris.empty()) {
        ZLOGI("Read to check uri authorization");
        std::map<std::string, int32_t> permissionUris;
        if (!CheckUriAuthorization(uris, tokenId, permissionUris)) {
            ZLOGE("UriAuth check failed:bundleName:%{public}s,tokenId:%{public}d,uris size:%{public}zu",
                  data.GetRuntime()->createPackage.c_str(), tokenId, uris.size());
            RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                BizScene::SET_DATA, SetDataStage::VERIFY_SHARE_PERMISSIONS, StageRes::FAILED, E_NO_PERMISSION);
            return E_NO_PERMISSION;
        }
        std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
        if (!IsNetworkingEnabled()) {
            FillUris(data, dfsUris, permissionUris);
            return E_OK;
        }
        int32_t userId;
        if (GetHapUidByToken(tokenId, userId) == E_OK) {
            GetDfsUrisFromLocal(uris, userId, dfsUris);
        }
        FillUris(data, dfsUris, permissionUris);
    }
    return E_OK;
}

int32_t PreProcessUtils::GetDfsUrisFromLocal(const std::vector<std::string> &uris, int32_t userId,
    std::unordered_map<std::string, HmdfsUriInfo> &dfsUris)
{
    DdsTrace trace(
        std::string(TAG) + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SET_DATA, SetDataStage::GERERATE_DFS_URI, StageRes::IDLE);
    int32_t ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(uris, userId, dfsUris);
    if (ret != 0 || dfsUris.empty()) {
        RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
            BizScene::SET_DATA, SetDataStage::GERERATE_DFS_URI, StageRes::FAILED, E_FS_ERROR, BizState::DFX_END);
        ZLOGE("Get remoteUri failed, ret = %{public}d, userId: %{public}d, uri size:%{public}zu.",
              ret, userId, uris.size());
        return E_FS_ERROR;
    }
    RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
        BizScene::SET_DATA, SetDataStage::GERERATE_DFS_URI, StageRes::SUCCESS);
    return E_OK;
}

void PreProcessUtils::FillUris(UnifiedData &data, std::unordered_map<std::string, HmdfsUriInfo> &dfsUris,
    std::map<std::string, int32_t> &permissionUris)
{
    if (permissionUris.empty()) {
        return;
    }
    ProcessFileType(data.GetRecords(), [&dfsUris, &permissionUris] (std::shared_ptr<Object> obj) {
        std::string oriUri;
        if (!obj->GetValue(ORI_URI, oriUri)) {
            return false;
        }
        if (permissionUris.find(oriUri) != permissionUris.end()) {
            obj->value_[PERMISSION_POLICY] = permissionUris[oriUri];
        } else {
            return true;
        }
        if (dfsUris.empty()) {
            return true;
        }
        auto iter = dfsUris.find(oriUri);
        if (iter != dfsUris.end()) {
            obj->value_[REMOTE_URI] = (iter->second).uriStr;
        }
        return true;
    });
    for (auto &record : data.GetRecords()) {
        if (record == nullptr) {
            continue;
        }
        record->ComputeUris([&dfsUris, &permissionUris] (UriInfo &uriInfo) {
            if (permissionUris.find(uriInfo.authUri) != permissionUris.end()) {
                uriInfo.permission = permissionUris[uriInfo.authUri];
            } else {
                return true;
            }
            if (dfsUris.empty()) {
                return true;
            }
            auto iter = dfsUris.find(uriInfo.authUri);
            if (iter != dfsUris.end()) {
                uriInfo.dfsUri = (iter->second).uriStr;
            }
            return true;
        });
    }
}

bool PreProcessUtils::CheckUriAuthorization(const std::vector<std::string>& uris, uint32_t tokenId,
    std::map<std::string, int32_t> &permissionUris)
{
    for (size_t index = 0; index < uris.size(); index += VERIFY_URI_PERMISSION_MAX_SIZE) {
        std::vector<std::string> urisToBeChecked(
            uris.begin() + index, uris.begin() + std::min(index + VERIFY_URI_PERMISSION_MAX_SIZE, uris.size()));
        auto checkResults = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
            urisToBeChecked, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
            AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION, tokenId);
        std::vector<std::string> noWritePermissionUris;
        for (size_t i = 0; i < checkResults.size(); ++i) {
            if (!checkResults[i]) {
                noWritePermissionUris.push_back(urisToBeChecked[i]);
                permissionUris[urisToBeChecked[i]] = static_cast<int32_t>(ONLY_READ);
            } else {
                permissionUris[urisToBeChecked[i]] = static_cast<int32_t>(READ_WRITE);
            }
        }
        if (!noWritePermissionUris.empty()) {
            auto results = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
                noWritePermissionUris, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, tokenId);
            auto item = find(results.begin(), results.end(), false);
            if (item != results.end()) {
                permissionUris.clear();
                return false;
            }
        }
        noWritePermissionUris.clear();
    }
    return true;
}

bool PreProcessUtils::GetInstIndex(uint32_t tokenId, int32_t &instIndex)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        instIndex = 0;
        return true;
    }
    HapTokenInfo tokenInfo;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("Get Hap TokenInfo error:%{public}d, tokenId:0x%{public}x", errCode, tokenId);
        return false;
    }
    instIndex = tokenInfo.instIndex;
    return true;
}

bool PreProcessUtils::IsNetworkingEnabled()
{
    std::vector<AppDistributedKv::DeviceInfo> devInfos =
        DistributedData::DeviceManagerAdapter::GetInstance().GetRemoteDevices();
    ZLOGI("Remote devices count:%{public}u", static_cast<uint32_t>(devInfos.size()));
    if (devInfos.empty()) {
        return false;
    }
    return true;
}

void PreProcessUtils::ProcessFileType(std::vector<std::shared_ptr<UnifiedRecord>> records,
    std::function<bool(std::shared_ptr<Object>)> callback)
{
    for (auto record : records) {
        if (record == nullptr) {
            ZLOGW("Record is empty!");
            continue;
        }
        auto entries = record->GetEntries();
        if (entries == nullptr) {
            ZLOGW("GetEntries returns empty!");
            continue;
        }
        auto entry = entries->find(GENERAL_FILE_URI);
        if (entry == entries->end()) {
            continue;
        }
        auto value = entry->second;
        if (!std::holds_alternative<std::shared_ptr<Object>>(value)) {
            continue;
        }
        auto obj = std::get<std::shared_ptr<Object>>(value);
        if (obj == nullptr) {
            ZLOGE("ValueType is not Object, Not convert to remote uri!");
            continue;
        }
        std::string dataType;
        if (obj->GetValue(UNIFORM_DATA_TYPE, dataType) && dataType == GENERAL_FILE_URI) {
            callback(obj);
        }
    }
}

void PreProcessUtils::ProcessRecord(std::shared_ptr<UnifiedRecord> record, uint32_t tokenId,
    bool isLocal, std::map<std::string, int32_t> &uris)
{
    record->ComputeUris([&uris, &isLocal, &tokenId] (UriInfo &uriInfo) {
        std::string newUriStr = "";
        if (isLocal && uriInfo.authUri.empty()) {
            Uri tmpUri(uriInfo.oriUri);
            std::string path = tmpUri.GetPath();
            std::string bundleName;
            if (!GetHapBundleNameByToken(tokenId, bundleName)) {
                return true;
            }
            if (path.substr(0, strlen(DOCS_LOCAL_TAG)) == DOCS_LOCAL_TAG) {
                newUriStr = FILE_SCHEME_PREFIX;
                newUriStr += path.substr(DOCS_LOCAL_PATH_SUBSTR_START_INDEX);
            } else {
                newUriStr = FILE_SCHEME_PREFIX;
                newUriStr += bundleName + path;
            }
            uriInfo.authUri = newUriStr;
        } else {
            newUriStr = isLocal ? uriInfo.authUri : uriInfo.dfsUri;
        }
        Uri uri(newUriStr);
        if (uri.GetAuthority().empty()) {
            return true;
        }
        std::string scheme = uri.GetScheme();
        std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
        if (scheme != FILE_SCHEME) {
            return true;
        }
        auto item = uris.find(newUriStr);
        if (item == uris.end()) {
            uris.emplace(newUriStr, uriInfo.permission);
        }
        return true;
    });
}

void PreProcessUtils::GetHtmlFileUris(uint32_t tokenId, UnifiedData &data,
    bool isLocal, std::map<std::string, int32_t> &htmlUris)
{
    if (data.HasType(UtdUtils::GetUtdIdFromUtdEnum(UDType::HTML))) {
        UnifiedHtmlRecordProcess::GetUriFromHtmlRecord(data);
    }
    for (auto &record : data.GetRecords()) {
        if (record == nullptr) {
            continue;
        }
        PreProcessUtils::ProcessRecord(record, tokenId, isLocal, htmlUris);
    }
}

void PreProcessUtils::ClearHtmlDfsUris(UnifiedData &data)
{
    for (auto &record : data.GetRecords()) {
        if (record == nullptr) {
            continue;
        }
        record->ComputeUris([] (UriInfo &uriInfo) {
            uriInfo.dfsUri = "";
            return true;
        });
    }
}

void PreProcessUtils::ProcessHtmlFileUris(uint32_t tokenId, UnifiedData &data, bool isLocal,
    std::vector<Uri> &readUris, std::vector<Uri> &writeUris)
{
    std::map<std::string, int32_t> strUris;
    for (auto &record : data.GetRecords()) {
        if (record == nullptr) {
            continue;
        }
        PreProcessUtils::ProcessRecord(record, tokenId, isLocal, strUris);
    }
    for (const auto &item : strUris) {
        if (item.second == PermissionPolicy::READ_WRITE || item.second == PermissionPolicy::UNKNOW) {
            writeUris.emplace_back(item.first);
        } else if (item.second == PermissionPolicy::ONLY_READ) {
            readUris.emplace_back(item.first);
        }
    }
    if (isLocal) {
        PreProcessUtils::ClearHtmlDfsUris(data);
    }
}

void PreProcessUtils::ProcessFiles(bool &hasError, UnifiedData &data, bool isLocal,
    std::vector<Uri> &readUris, std::vector<Uri> &writeUris)
{
    PreProcessUtils::ProcessFileType(data.GetRecords(), [&] (std::shared_ptr<Object> obj) {
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
        if (!ValidateUriScheme(uri, hasError)) {
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
            if (!ValidateUriScheme(uri, hasError)) {
                return false;
            }
        }
        int32_t permission;
        if (obj->GetValue(PERMISSION_POLICY, permission)) {
            permission == PermissionPolicy::READ_WRITE ? writeUris.emplace_back(uri) : readUris.emplace_back(uri);
            return true;
        }
        writeUris.emplace_back(uri); // Compatibility Handling Between Different Versions.
        return true;
    });
}

bool PreProcessUtils::ValidateUriScheme(Uri &uri, bool &hasError)
{
    std::string scheme = uri.GetScheme();
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
    if (!scheme.empty() && scheme != FILE_SCHEME) {
        ZLOGW("scheme is not file");
        return false;
    }

    if (scheme.empty() || uri.GetAuthority().empty()) {
        hasError = true;
        ZLOGE("Empty authority or scheme not file");
        return false;
    }
    return true;
}

void PreProcessUtils::SetRecordUid(UnifiedData &data)
{
    uint32_t index = 0;
    auto prefix = PreProcessUtils::GenerateId().substr(0, PREFIX_LEN);
    for (const auto &record : data.GetRecords()) {
        if (record == nullptr) {
            continue;
        }
        std::ostringstream oss;
        oss << std::setw(INDEX_LEN) << std::setfill(PLACE_HOLDER) << index;
        record->SetUid(prefix + oss.str());
        index++;
    }
}

bool PreProcessUtils::GetDetailsFromUData(const UnifiedData &data, UDDetails &details)
{
    auto records = data.GetRecords();
    if (records.size() != TEMP_UDATA_RECORD_SIZE) {
        ZLOGI("Records size:%{public}zu", records.size());
        return false;
    }
    if (records[0] == nullptr) {
        ZLOGE("First record is null.");
        return false;
    }
    if (records[0]->GetType() != UDType::FILE) {
        ZLOGE("First record is not file.");
        return false;
    }
    auto value = records[0]->GetOriginValue();
    auto obj = std::get_if<std::shared_ptr<Object>>(&value);
    if (obj == nullptr || *obj == nullptr) {
        ZLOGE("ValueType is not Object!");
        return false;
    }
    std::shared_ptr<Object> detailObj;
    (*obj)->GetValue(DETAILS, detailObj);
    if (detailObj == nullptr) {
        ZLOGE("Not contain details for object!");
        return false;
    }
    auto result = ObjectUtils::ConvertToUDDetails(detailObj);
    if (result.find(TEMP_UNIFIED_DATA_FLAG) == result.end()) {
        ZLOGE("Not find temp file.");
        return false;
    }
    details = std::move(result);
    return true;
}

Status PreProcessUtils::GetSummaryFromDetails(const UDDetails &details, Summary &summary)
{
    for (auto &item : details) {
        if (item.first == TEMP_UNIFIED_DATA_FLAG) {
            continue;
        }
        auto int64Value = std::get_if<int64_t>(&item.second);
        if (int64Value != nullptr) {
            summary.summary[item.first] = *int64Value;
            summary.totalSize += *int64Value;
        }
    }
    return E_OK;
}

std::string PreProcessUtils::GetSdkVersionByToken(uint32_t tokenId)
{
    if (Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId) !=
        Security::AccessToken::ATokenTypeEnum::TOKEN_HAP) {
        ZLOGE("Caller is not application, tokenid is %{public}u", tokenId);
        return "";
    }
    Security::AccessToken::HapTokenInfo hapTokenInfo;
    auto ret = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (ret != 0) {
        ZLOGE("GetHapTokenInfo fail, tokenid is %{public}u, ret is %{public}d.", tokenId, ret);
        return "";
    }
    return std::to_string(hapTokenInfo.apiVersion);
}

bool PreProcessUtils::GetSpecificBundleNameByTokenId(uint32_t tokenId, std::string &specificBundleName,
    std::string &bundleName)
{
    if (UTILS::IsTokenNative(tokenId)) {
        ZLOGI("TypeATokenTypeEnum is TOKEN_NATIVE");
        std::string processName;
        if (GetNativeProcessNameByToken(tokenId, processName)) {
            bundleName = std::move(processName);
            specificBundleName = bundleName;
            return true;
        }
    }
    Security::AccessToken::HapTokenInfo hapInfo;
    if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo)
        == Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        bundleName = hapInfo.bundleName;
        return GetSpecificBundleName(hapInfo.bundleName, hapInfo.instIndex, specificBundleName);
    }
    ZLOGE("Get bundle name faild, tokenid:%{public}u", tokenId);
    return false;
}

sptr<AppExecFwk::IBundleMgr> PreProcessUtils::GetBundleMgr()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        ZLOGE("Failed to get system ability mgr.");
        return nullptr;
    }
    auto bundleMgrProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleMgrProxy == nullptr) {
        ZLOGE("Failed to Get BMS SA.");
        return nullptr;
    }
    auto bundleManager = iface_cast<AppExecFwk::IBundleMgr>(bundleMgrProxy);
    if (bundleManager == nullptr) {
        ZLOGE("Failed to get bundle manager");
        return nullptr;
    }
    return bundleManager;
}

bool PreProcessUtils::GetSpecificBundleName(const std::string &bundleName, int32_t appIndex,
    std::string &specificBundleName)
{
    auto bundleManager = GetBundleMgr();
    if (bundleManager == nullptr) {
        ZLOGE("Failed to get bundle manager, bundleName:%{public}s, appIndex:%{public}d", bundleName.c_str(), appIndex);
        return false;
    }
    auto ret = bundleManager->GetDirByBundleNameAndAppIndex(bundleName, appIndex, specificBundleName);
    if (ret != ERR_OK) {
        ZLOGE("GetDirByBundleNameAndAppIndex failed, ret:%{public}d, bundleName:%{public}s, appIndex:%{public}d",
            ret, bundleName.c_str(), appIndex);
        return false;
    }
    return true;
}
} // namespace UDMF
} // namespace OHOS
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

#include <random>
#include <sstream>

#include "accesstoken_kit.h"
#include "bundlemgr/bundle_mgr_client_impl.h"
#include "device_manager_adapter.h"
#include "error_code.h"
#include "file.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "remote_file_share.h"
#include "uri.h"
#include "utils/crypto.h"
#include "want.h"
#include "uri_permission_manager_client.h"
namespace OHOS {
namespace UDMF {
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;
static constexpr int MAXIMUM = 121;
const char SPECIAL = '^';
static constexpr uint32_t VERIFY_URI_PERMISSION_MAX_SIZE = 500;
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;

int32_t PreProcessUtils::RuntimeDataImputation(UnifiedData &data, CustomOption &option)
{
    auto it = UD_INTENTION_MAP.find(option.intention);
    if (it == UD_INTENTION_MAP.end()) {
        return E_ERROR;
    }
    std::string bundleName;
    GetHapBundleNameByToken(option.tokenId, bundleName);
    std::string intention = it->second;
    UnifiedKey key(intention, bundleName, GenerateId());
    Privilege privilege;
    privilege.tokenId = option.tokenId;
    Runtime runtime;
    runtime.key = key;
    runtime.privileges.emplace_back(privilege);
    runtime.createTime = GetTimestamp();
    runtime.sourcePackage = bundleName;
    runtime.createPackage = bundleName;
    runtime.deviceId = GetLocalDeviceId();
    runtime.recordTotalNum = static_cast<uint32_t>(data.GetRecords().size());
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

int32_t PreProcessUtils::GetHapUidByToken(uint32_t tokenId)
{
    Security::AccessToken::HapTokenInfo tokenInfo;
    auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result != Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        ZLOGE("GetHapUidByToken failed, result = %{public}d.", result);
        return E_ERROR;
    }
    return tokenInfo.userID;
}

bool PreProcessUtils::GetHapBundleNameByToken(int tokenId, std::string &bundleName)
{
    Security::AccessToken::HapTokenInfo hapInfo;
    if (Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo)
        != Security::AccessToken::AccessTokenKitRet::RET_SUCCESS) {
        return false;
    }
    bundleName = hapInfo.bundleName;
    return true;
}

bool PreProcessUtils::GetNativeProcessNameByToken(int tokenId, std::string &processName)
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
    for (auto record : records) {
        auto type = record->GetType();
        if (IsFileType(type)) {
            auto file = static_cast<File *>(record.get());
            UDDetails details = file->GetDetails();
            details.insert({ "isRemote", "true" });
            file->SetDetails(details);
        }
    }
}

bool PreProcessUtils::IsFileType(UDType udType)
{
    return (udType == UDType::FILE || udType == UDType::IMAGE || udType == UDType::VIDEO || udType == UDType::AUDIO
        || udType == UDType::FOLDER);
}

int32_t PreProcessUtils::SetRemoteUri(uint32_t tokenId, UnifiedData &data)
{
    int32_t userId = GetHapUidByToken(tokenId);
    std::vector<std::string> uris;
    for (const auto &record : data.GetRecords()) {
        if (record != nullptr && IsFileType(record->GetType())) {
            auto file = static_cast<File *>(record.get());
            if (file->GetUri().empty()) {
                ZLOGW("Get uri empty, plase check the uri.");
                continue;
            }
            Uri uri(file->GetUri());
            if (uri.GetAuthority().empty()) {
                ZLOGW("Get uri authority empty.");
                continue;
            }
            uris.push_back(file->GetUri());
        }
    }
    if (!uris.empty()) {
        if (!CheckUriAuthorization(uris, tokenId)) {
            ZLOGE("CheckUriAuthorization failed, bundleName:%{public}s, tokenId: %{public}d, uris size:%{public}zu.",
                  data.GetRuntime()->createPackage.c_str(), tokenId, uris.size());
            return E_NO_PERMISSION;
        }
        int ret = GetDfsUrisFromLocal(uris, userId, data);
        if (ret != E_OK) {
            ZLOGE("Get remoteUri failed, ret = %{public}d, userId: %{public}d, uri size:%{public}zu.",
                  ret, userId, uris.size());
            return E_FS_ERROR;
        }
    }
    return E_OK;
}

int32_t PreProcessUtils::GetDfsUrisFromLocal(const std::vector<std::string> &uris, int32_t userId, UnifiedData &data)
{
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    int ret = RemoteFileShare::GetDfsUrisFromLocal(uris, userId, dfsUris);
    if (ret != 0 || dfsUris.empty()) {
        ZLOGE("Get remoteUri failed, ret = %{public}d, userId: %{public}d, uri size:%{public}zu.",
              ret, userId, uris.size());
        return E_FS_ERROR;
    }
    for (const auto &record : data.GetRecords()) {
        if (record != nullptr && IsFileType(record->GetType())) {
            auto file = static_cast<File *>(record.get());
            auto iter = dfsUris.find(file->GetUri());
            if (iter != dfsUris.end()) {
                file->SetRemoteUri((iter->second).uriStr);
            }
        }
    }
    return E_OK;
}

bool PreProcessUtils::CheckUriAuthorization(const std::vector<std::string>& uris, uint32_t tokenId)
{
    for (size_t index = 0; index < uris.size(); index += VERIFY_URI_PERMISSION_MAX_SIZE) {
        std::vector<std::string> urisToBeChecked(
            uris.begin() + index, uris.begin() + std::min(index + VERIFY_URI_PERMISSION_MAX_SIZE, uris.size()));
        auto checkResults = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
            urisToBeChecked, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, tokenId);
        auto iter = find(checkResults.begin(), checkResults.end(), false);
        if (iter != checkResults.end()) {
            return false;
        }
    }
    return true;
}
} // namespace UDMF
} // namespace OHOS
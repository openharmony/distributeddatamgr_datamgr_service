/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "bundle_info.h"
#include "dds_trace.h"
#include "udmf_radar_reporter.h"
#include "accesstoken_kit.h"
#include "device_manager_adapter.h"
#include "file_mount_manager.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#include "udmf_radar_reporter.h"
#include "udmf_utils.h"
#include "utils/crypto.h"
#include "uri_permission_manager_client.h"
#include "ipc_skeleton.h"
#include "bundlemgr/bundle_mgr_interface.h"
namespace OHOS {
namespace UDMF {
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
    std::string bundleName = "bundleName";
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
    runtime.recordTotalNum = static_cast<uint32_t>(data.GetRecords().size());
    runtime.tokenId = option.tokenId;
    runtime.visibility = option.visibility;
    runtime.appId = "appId";
    data.SetRuntime(runtime);
    return E_OK;
}

int32_t PreProcessUtils::FillDelayRuntimeInfo(UnifiedData &data, CustomOption &option, const DataLoadInfo &info)
{
    std::string bundleName = "bundleName";
    UnifiedKey key(UD_INTENTION_MAP.at(UD_INTENTION_DRAG), bundleName, info.sequenceKey);
    Privilege privilege;
    privilege.tokenId = option.tokenId;
    
    Runtime runtime;
    runtime.key = key;
    runtime.privileges.emplace_back(privilege);
    runtime.createTime = GetTimestamp();
    runtime.sourcePackage = bundleName;
    runtime.createPackage = bundleName;
    runtime.recordTotalNum = info.recordCount;
    runtime.tokenId = option.tokenId;
    runtime.visibility = option.visibility;
    runtime.appId = "appId";
    runtime.dataStatus = DataStatus::WAITING;
    data.SetRuntime(runtime);
    return E_OK;
}

std::string PreProcessUtils::GenerateId()
{
    return "1122ac";
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
    return E_OK;
}

bool PreProcessUtils::GetHapBundleNameByToken(uint32_t tokenId, std::string &bundleName)
{
    bundleName = "processName";
    return true;
}

bool PreProcessUtils::GetNativeProcessNameByToken(uint32_t tokenId, std::string &processName)
{
    processName = "processName";
    return true;
}

std::string PreProcessUtils::GetLocalDeviceId()
{
    return "123";
}

void PreProcessUtils::SetRemoteData(UnifiedData &data)
{
    return;
}

int32_t PreProcessUtils::HandleFileUris(uint32_t tokenId, UnifiedData &data)
{
    return E_OK;
}

bool PreProcessUtils::GetInstIndex(uint32_t tokenId, int32_t &instIndex)
{
    return true;
}

bool PreProcessUtils::IsNetworkingEnabled()
{
    return true;
}

void PreProcessUtils::ProcessFileType(std::vector<std::shared_ptr<UnifiedRecord>> records,
    std::function<bool(std::shared_ptr<Object>)> callback)
{
    return;
}

void PreProcessUtils::ProcessRecord(std::shared_ptr<UnifiedRecord> record, uint32_t tokenId,
    bool isLocal, std::map<std::string, int32_t> &uris)
{
    return;
}

void PreProcessUtils::GetHtmlFileUris(uint32_t tokenId, UnifiedData &data, bool isLocal,
    std::map<std::string, int32_t> &htmlUris)
{
    return;
}

void PreProcessUtils::ClearHtmlDfsUris(UnifiedData &data)
{
    return;
}

void PreProcessUtils::ProcessFiles(bool &hasError, UnifiedData &data, bool isLocal,
    std::vector<Uri> &readUris, std::vector<Uri> &writeUris)
{
    return;
}


void PreProcessUtils::ProcessHtmlFileUris(uint32_t tokenId, UnifiedData &data, bool isLocal,
    std::vector<Uri> &readUris, std::vector<Uri> &writeUris)
{
    return;
}

void PreProcessUtils::SetRecordUid(UnifiedData &data)
{
    return;
}

bool PreProcessUtils::GetDetailsFromUData(const UnifiedData &data, UDDetails &details)
{
    return true;
}

Status PreProcessUtils::GetSummaryFromDetails(const UDDetails &details, Summary &summary)
{
    return E_OK;
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
    return true;
}

bool PreProcessUtils::GetSpecificBundleNameByTokenId(uint32_t tokenId, std::string &specificBundleName,
    std::string &bundleName)
{
    specificBundleName = "specificBundleName";
    bundleName = "bundleName";
    return true;
}
} // namespace UDMF
} // namespace OHOS
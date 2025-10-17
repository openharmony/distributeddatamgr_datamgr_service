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
#ifndef UDMF_PREPROCESS_UTILS_H
#define UDMF_PREPROCESS_UTILS_H

#include "bundlemgr/bundle_mgr_proxy.h"
#include "remote_file_share.h"
#include "unified_data.h"

namespace OHOS {
namespace UDMF {
class PreProcessUtils {
public:
    static int32_t FillRuntimeInfo(UnifiedData &data, CustomOption &option);
    static std::string GenerateId();
    static time_t GetTimestamp();
    static int32_t GetHapUidByToken(uint32_t tokenId, int &userId);
    static bool GetHapBundleNameByToken(uint32_t tokenId, std::string &bundleName);
    static bool GetNativeProcessNameByToken(uint32_t tokenId, std::string &processName);
    static std::string GetLocalDeviceId();
    static void SetRemoteData(UnifiedData &data);
    static int32_t HandleFileUris(uint32_t tokenId, UnifiedData &data);
    static bool GetInstIndex(uint32_t tokenId, int32_t &instIndex);
    static bool IsNetworkingEnabled();
    static void ProcessFileType(std::vector<std::shared_ptr<UnifiedRecord>> records,
        std::function<bool(std::shared_ptr<Object>)> callback);
    static void GetHtmlFileUris(uint32_t tokenId, UnifiedData &data, bool isLocal,
        std::map<std::string, int32_t> &htmlUris);
    static void ClearHtmlDfsUris(UnifiedData &data);
    static void ProcessHtmlFileUris(uint32_t tokenId, UnifiedData &data, bool isLocal,
        std::vector<Uri> &readUris, std::vector<Uri> &writeUris);
    static void ProcessFiles(bool &hasError, UnifiedData &data, bool isLocal,
        std::vector<Uri> &readUris, std::vector<Uri> &writeUris);
    static void ProcessRecord(std::shared_ptr<UnifiedRecord> record, uint32_t tokenId,
        bool isLocal, std::map<std::string, int32_t> &uris);
    static void SetRecordUid(UnifiedData &data);
    static bool GetDetailsFromUData(const UnifiedData &data, UDDetails &details);
    static Status GetSummaryFromDetails(const UDDetails &details, Summary &summary);
    static bool GetSpecificBundleNameByTokenId(uint32_t tokenId, std::string &specificBundleName,
        std::string &bundleName);
    static sptr<AppExecFwk::IBundleMgr> GetBundleMgr();
private:
    static bool CheckUriAuthorization(const std::vector<std::string>& uris, uint32_t tokenId,
        std::map<std::string, int32_t> &permissionUris);
    static int32_t GetDfsUrisFromLocal(const std::vector<std::string> &uris, int32_t userId,
        std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> &dfsUris);
    static std::string GetSdkVersionByToken(uint32_t tokenId);
    static bool GetSpecificBundleName(const std::string &bundleName, int32_t appIndex, std::string &specificBundleName);
    static void FillUris(UnifiedData &data,
        std::unordered_map<std::string, AppFileService::ModuleRemoteFileShare::HmdfsUriInfo> &dfsUris,
        std::map<std::string, int32_t> &permissionUris);
    static int32_t ReadCheckUri(uint32_t tokenId, UnifiedData &data, std::vector<std::string> &uris);
    static bool ValidateUriScheme(Uri &uri, bool &hasError);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_PREPROCESS_UTILS_H
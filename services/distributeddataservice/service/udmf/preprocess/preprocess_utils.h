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

#include <bitset>
#include <random>
#include <string>
#include <vector>

#include "unified_data.h"
#include "unified_meta.h"

namespace OHOS {
namespace UDMF {
class PreProcessUtils {
public:
    static int32_t RuntimeDataImputation(UnifiedData &data, CustomOption &option);
    static std::string GenerateId();
    static time_t GetTimestamp();
    static int32_t GetHapUidByToken(uint32_t tokenId);
    static bool GetHapBundleNameByToken(int tokenId, std::string &bundleName);
    static bool GetNativeProcessNameByToken(int tokenId, std::string &processName);
    static std::string GetLocalDeviceId();
    static void SetRemoteData(UnifiedData &data);
    static bool IsFileType(UDType udType);
    static int32_t SetRemoteUri(uint32_t tokenId, UnifiedData &data);
private:
    static bool CheckUriAuthorization(const std::vector<std::string>& uris, uint32_t tokenId);
    static int32_t GetDfsUrisFromLocal(const std::vector<std::string> &uris, int32_t userId, UnifiedData &data);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_PREPROCESS_UTILS_H
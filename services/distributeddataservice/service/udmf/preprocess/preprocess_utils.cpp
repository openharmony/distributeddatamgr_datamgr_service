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

#include "preprocess_utils.h"

#include <sstream>

#include "error_code.h"
#include "accesstoken_kit.h"
#include "bundlemgr/bundle_mgr_client_impl.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace UDMF {
static constexpr int ID_LEN = 32;
const char SPECIAL = '^';
int32_t PreProcessUtils::RuntimeDataImputation(UnifiedData &data, CustomOption &option)
{
    auto it = UD_INTENTION_MAP.find(option.intention);
    if (it == UD_INTENTION_MAP.end()) {
        return E_UNKNOWN;
    }
    std::string bundleName;
    GetHapBundleNameByToken(option.tokenId, bundleName);
    std::string intention = it->second;
    UnifiedKey key(intention, bundleName, IdGenerator());
    Privilege privilege;
    privilege.tokenId = option.tokenId;
    Runtime runtime;
    runtime.key = key;
    runtime.privileges.emplace_back(privilege);
    runtime.createTime = GetTimeStamp();
    runtime.sourcePackage = bundleName;
    runtime.createPackage = bundleName;
    data.SetRuntime(runtime);
    return E_OK;
}

std::string PreProcessUtils::IdGenerator()
{
    std::random_device randomDevice;
    int minimum = 48;
    int maximum = 121;
    std::uniform_int_distribution<int> distribution(minimum, maximum);
    std::stringstream idStr;
    for (int32_t i = 0; i < ID_LEN; i++) {
        auto asc = distribution(randomDevice);
        asc = asc >= SPECIAL ? asc + 1 : asc;
        idStr << static_cast<uint8_t>(asc);
    }
    return idStr.str();
}

time_t PreProcessUtils::GetTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    time_t timestamp = tp.time_since_epoch().count();
    return timestamp;
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
} // namespace UDMF
} // namespace OHOS
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

#include "cloudservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "accesstoken_kit.h"
#include "cloud_service_impl.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"

using namespace OHOS::CloudData;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
constexpr int USER_ID = 100;
constexpr int INST_INDEX = 0;
const std::u16string INTERFACE_TOKEN = u"OHOS.CloudData.CloudServer";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = CloudService::TransId::TRANS_BUTT + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

void AllocAndSetHapToken()
{
    HapInfoParams info = {
        .userID = USER_ID,
        .bundleName = "ohos.test.demo",
        .instIndex = INST_INDEX,
        .appIDDesc = "ohos.test.demo",
        .isSystemApp = true
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {},
        .permStateList = {
            {
                .permissionName = "ohos.permission.CLOUDDATA_CONFIG",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}

bool OnRemoteRequestFuzz(const uint8_t *data, size_t size)
{
    std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    cloudServiceImpl->OnBind(
        { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    AllocAndSetHapToken();

    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::OnRemoteRequestFuzz(data, size);

    return 0;
}
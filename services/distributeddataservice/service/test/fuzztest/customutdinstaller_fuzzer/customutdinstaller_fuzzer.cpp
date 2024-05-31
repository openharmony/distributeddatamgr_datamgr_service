/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "customutdinstaller_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "ipc_skeleton.h"
#include "custom_utd_installer.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::UDMF;

namespace OHOS {

bool InstallUtdFuzz(const uint8_t* data, size_t size)
{
    int32_t user = static_cast<int32_t>(*data);
    const std::string bundleName = "com.example.myapplication";
    auto ret = CustomUtdInstaller::GetInstance().InstallUtd(bundleName, user);
    if (ret != E_OK) {
        return false;
    } else {
        return true;
    }
}

bool UninstallUtdFuzz(const uint8_t* data, size_t size)
{
    bool result = false;
    int32_t user = static_cast<int32_t>(*data);
    std::string bundleName = "com.example.myapplication";
    auto ret = CustomUtdInstaller::GetInstance().UninstallUtd(bundleName, user);
    if (!ret) {
        result = true;
    }
    return result;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }

    OHOS::InstallUtdFuzz(data, size);
    OHOS::UninstallUtdFuzz(data, size);

    return 0;
}
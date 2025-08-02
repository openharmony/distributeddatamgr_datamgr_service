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

#include "bundle_utils.h"

namespace OHOS::DistributedData {
std::shared_ptr<Bundle_Utils> Bundle_Utils::GetInstance()
{
    static std::shared_ptr<Bundle_Utils> instance = nullptr;
    if (instance == nullptr) {
        instance = std::make_shared<Bundle_Utils>();
    }
    return instance;
}

void Bundle_Utils::SetBundleInfoCallback(Callback callback)
{
    std::lock_guard<std::mutex> lock(lock_);
    bundleInfoCallback_ = callback;
}

std::pair<int, bool> Bundle_Utils::CheckSilentConfig(const std::string &bundleName, int32_t userId)
{
    std::lock_guard<std::mutex> lock(lock_);
    if (bundleInfoCallback_ == nullptr) {
        return std::make_pair(-1, false);
    }
    return bundleInfoCallback_(bundleName, userId);
}
}
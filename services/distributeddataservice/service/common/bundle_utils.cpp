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
BundleUtils &BundleUtils::GetInstance()
{
    static BundleUtils instance;
    return instance;
}

void BundleUtils::SetBundleInfoCallback(Callback callback)
{
    std::lock_guard<std::mutex> lock(lock_);
    bundleInfoCallback_ = std::move(callback);
}

std::pair<int, bool> BundleUtils::CheckSilentConfig(const std::string &bundleName, int32_t userId)
{
    Callback callback;
    {
        std::lock_guard<std::mutex> lock(lock_);
        callback = bundleInfoCallback_;
    }
    if (callback == nullptr) {
        return std::make_pair(-1, false);
    }
    return callback(bundleName, userId);
}
}
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

#define LOG_TAG "BundleMgrAdapter"
#include "bundle_mgr/bundle_mgr_adapter.h"
#include "bundle_mgr_client.h"
#include "log_print.h"
using namespace OHOS::AppExecFwk;
namespace OHOS::DistributedData {

BundleMgrAdapter &BundleMgrAdapter::GetInstance()
{
    static BundleMgrAdapter instance;
    return instance;
}

std::string BundleMgrAdapter::GetBundleName(int32_t uid)
{
    BundleMgrClient client;
    std::string bundleName;
    auto code = client.GetNameForUid(uid, bundleName);
    if (code != 0) {
        ZLOGE("failed. uid:%{public}d, code:%{public}d", uid, code);
        return "";
    }
    return bundleName;
}
} // namespace OHOS::DistributedData
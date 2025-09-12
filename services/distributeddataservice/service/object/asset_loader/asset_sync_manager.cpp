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
#define LOG_TAG "AssetSyncManager"
#include "asset_sync_manager.h"

#include <cinttypes>

#include "block_data.h"
#include "cloud_sync_asset_manager.h"
#include "log_print.h"
#include "object_common.h"
#include "utils/anonymous.h"
#include "visibility.h"

namespace OHOS::DistributedObject {
using namespace OHOS::FileManagement::CloudSync;
using namespace OHOS::DistributedData;
static constexpr int WAIT_TIME = 60;
bool AssetSyncManager::Transfer(const int32_t userId, const std::string &bundleName, const std::string &deviceId,
    const DistributedData::Asset &asset)
{
    AssetInfo assetInfo;
    assetInfo.uri = asset.uri;
    assetInfo.assetName = asset.name;
    ZLOGI("Start transfer, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
        Anonymous::Change(deviceId).c_str(), Anonymous::Change(assetInfo.assetName).c_str());
    auto block = std::make_shared<BlockData<std::tuple<bool, int32_t>>>(WAIT_TIME, std::tuple{ true, OBJECT_SUCCESS });
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [block](const std::string &uri, int32_t status) { block->SetValue({ false, status }); });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("fail, res: %{public}d, name: %{public}s, deviceId: %{public}s, bundleName: %{public}s", res,
            Anonymous::Change(asset.name).c_str(), Anonymous::Change(deviceId).c_str(), bundleName.c_str());
        return false;
    }
    auto [timeout, status] = block->GetValue();
    if (timeout || status != OBJECT_SUCCESS) {
        ZLOGE("fail, timeout: %{public}d, status: %{public}d, name: %{public}s, deviceId: %{public}s ", timeout,
            status, Anonymous::Change(asset.name).c_str(), Anonymous::Change(deviceId).c_str());
        return false;
    }
    ZLOGD("Transfer end, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
        Anonymous::Change(deviceId).c_str(), Anonymous::Change(assetInfo.assetName).c_str());
    return true;
}
} // namespace OHOS::DistributedObject

API_EXPORT std::shared_ptr<OHOS::DistributedObject::IAssetSyncManager> Create() asm("CreateObjectAssetSyncManager");
std::shared_ptr<OHOS::DistributedObject::IAssetSyncManager> Create()
{
    return std::make_shared<OHOS::DistributedObject::AssetSyncManager>();
}
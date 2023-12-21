/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "ObjectAssetLoader"

#include "object_asset_loader.h"
#include "block_data.h"
#include "cloud_sync_asset_manager.h"
#include "log_print.h"
#include "object_common.h"

namespace OHOS::DistributedObject {
using namespace OHOS::FileManagement::CloudSync;
ObjectAssetLoader *ObjectAssetLoader::GetInstance()
{
    static ObjectAssetLoader *loader = new ObjectAssetLoader();
    return loader;
}

bool ObjectAssetLoader::Transfer(const int32_t userId, const std::string &bundleName,
    const std::string &deviceId, const DistributedData::Asset &assetValue)
{
    AssetInfo assetInfo;
    assetInfo.uri = assetValue.uri;
    assetInfo.assetName = assetValue.name;
    ZLOGD("start transfer file, userId: %{public}d, bundleName: %{public}s, networkId: %{public}s, asset name : "
          "%{public}s", userId, bundleName.c_str(), deviceId.c_str(), assetValue.name.c_str());

    auto block = std::make_shared<BlockData<std::tuple<bool, int32_t>>>(WAIT_TIME, std::tuple{ true, OBJECT_SUCCESS });
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [block](const std::string &uri, int32_t status) {
            block->SetValue({ false, status });
        });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("Transfer file fail, bundleName: %{public}s, asset name : %{public}s, result: %{public}d",
            bundleName.c_str(), assetValue.name.c_str(), res);
        return false;
    }
    auto [timeout, status] = block->GetValue();
    if (timeout || status != OBJECT_SUCCESS) {
        ZLOGE("Transfer file fail, bundleName: %{public}s, asset name : %{public}s, timeout: %{public}d, status: "
              "%{public}d", bundleName.c_str(), assetValue.name.c_str(), timeout, status);
        return false;
    }
    return true;
}

bool ObjectAssetLoader::Transfer(const int32_t userId, const std::string& bundleName, const std::string& deviceId,
    const DistributedData::Asset& assetValue, std::function<void(bool success)> callback)
{
    AssetInfo assetInfo;
    assetInfo.uri = assetValue.uri;
    assetInfo.assetName = assetValue.name;
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [callback](const std::string& uri, int32_t status) {
            status == OBJECT_SUCCESS ? callback(true) : callback(false);
        });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("Transfer file fail, bundleName: %{public}s, asset name : %{public}s, result: %{public}d",
            bundleName.c_str(), assetValue.name.c_str(), res);
        return false;
    }
    return true;
}
} // namespace OHOS::DistributedObject
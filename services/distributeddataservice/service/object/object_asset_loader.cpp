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
#include "utils/anonymous.h"

namespace OHOS::DistributedObject {
using namespace OHOS::FileManagement::CloudSync;
ObjectAssetLoader *ObjectAssetLoader::GetInstance()
{
    static ObjectAssetLoader *loader = new ObjectAssetLoader();
    return loader;
}

void ObjectAssetLoader::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
   executors_ = executors;
}

bool ObjectAssetLoader::Transfer(const int32_t userId, const std::string &bundleName,
    const std::string &deviceId, const DistributedData::Asset &assetValue)
{
    AssetInfo assetInfo;
    assetInfo.uri = assetValue.uri;
    assetInfo.assetName = assetValue.name;
    ZLOGD("Start transfer, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
          DistributedData::Anonymous::Change(deviceId).c_str(), assetInfo.assetName.c_str());
    auto block = std::make_shared<BlockData<std::tuple<bool, int32_t>>>(WAIT_TIME, std::tuple{ true, OBJECT_SUCCESS });
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [block](const std::string &uri, int32_t status) {
            block->SetValue({ false, status });
        });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("fail, res: %{public}d, name: %{public}s, deviceId: %{public}s, bundleName: %{public}s", res,
            assetValue.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str(), bundleName.c_str());
        return false;
    }
    auto [timeout, status] = block->GetValue();
    if (timeout || status != OBJECT_SUCCESS) {
        ZLOGE("fail, timeout: %{public}d, status: %{public}d, name: %{public}s, deviceId: %{public}s ", timeout,
            status, assetValue.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str());
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
            if (status != OBJECT_SUCCESS) {
                ZLOGE("fail, uri: %{public}s, status: %{public}d", DistributedData::Anonymous::Change(uri).c_str(),
                    status);
                return callback(false);
            }
            return callback(true);
        });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("fail, res: %{public}d, name: %{public}s, deviceId: %{public}s, bundleName: %{public}s", res,
            assetValue.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str(), bundleName.c_str());
        return false;
    }
    return true;
}

void ObjectAssetLoader::TransferAssets(const int32_t userId, const std::string& bundleName, const std::string& deviceId,
   const std::vector<DistributedData::Asset>& assets, const std::function<void(bool success)>& callback)
{
   executors_->Execute([this, userId, bundleName, deviceId, assets, callback]() {
       bool result = true;
       for (auto& asset : assets) {
           result &= Transfer(userId, bundleName, deviceId, asset);
       }
       callback(result);
   });
}
} // namespace OHOS::DistributedObject
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
    const std::string &deviceId, const DistributedData::Asset & asset)
{
    auto downloading = downloading_.ComputeIfAbsent(asset.uri,[asset](const std::string& key){
        return asset.hash;
    });
    if (!downloading) {
        return true;
    }

    AssetInfo assetInfo;
    assetInfo.uri = asset.uri;
    assetInfo.assetName = asset.name;
    ZLOGI("Start transfer, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
          DistributedData::Anonymous::Change(deviceId).c_str(), assetInfo.assetName.c_str());
    auto block = std::make_shared<BlockData<std::tuple<bool, int32_t>>>(WAIT_TIME, std::tuple{ true, OBJECT_SUCCESS });
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [block](const std::string &uri, int32_t status) {
            block->SetValue({ false, status });
        });
    CheckCallcack(asset.uri, res);
    if (res != OBJECT_SUCCESS) {
        ZLOGE("fail, res: %{public}d, name: %{public}s, deviceId: %{public}s, bundleName: %{public}s", res,
            asset.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str(), bundleName.c_str());
        return false;
    }
    auto [timeout, status] = block->GetValue();
    if (timeout || status != OBJECT_SUCCESS) {
        ZLOGE("fail, timeout: %{public}d, status: %{public}d, name: %{public}s, deviceId: %{public}s ", timeout,
            status, asset.name.c_str(), DistributedData::Anonymous::Change(deviceId).c_str());
        return false;
    }

    // �������
    downloaded_.ComputeIfAbsent(asset.uri,[asset](const std::string& key){
        return asset.hash;
    });
    downloaded_[asset.uri] = asset.hash;
    downloading_.Erase(asset.uri);

    std::lock_guard<std::mutex> lock(mutex);
    assetQueue_.push(asset.uri);
    if (assetQueue_.size() > LAST_DOWNLOAD_ASSET_SIZE) {
        auto oldAsset = assetQueue_.front();
        assetQueue_.pop();
        downloaded_.Erase(oldAsset);
    }
    return true;
}

void ObjectAssetLoader::TransferAssetsAsync(const int32_t userId, const std::string& bundleName,
    const std::string& deviceId, const std::vector<DistributedData::Asset>& assets,
    const std::function<void(bool success)>& callback)
{
    if (executors_ == nullptr) {
        ZLOGE("executors is null, bundleName: %{public}s, deviceId: %{public}s, userId: %{public}d",
            bundleName.c_str(), DistributedData::Anonymous::Change(deviceId).c_str(), userId);
        callback(false);
        return;
    }

    TransferTask task;
    task.callback = callback;

    for (auto &asset: assets) {
        if (downloaded_[asset.uri] == asset.hash) {
            continue;
        }
        task.downloadAssets.insert(asset.uri);
    }
    taskSeq_++;
    tasks_.ComputeIfAbsent(taskSeq_,[task](const uint32_t key){
        return task;
    });

    executors_->Execute([this, userId, bundleName, deviceId, assets, callback]() {
        for (const auto& asset : assets) {
            Transfer(userId, bundleName, deviceId, asset);
        }
    });

}
void ObjectAssetLoader::CheckCallcack(const std::string& uri, bool result)
{
    tasks_.ForEach([&uri, result, this](auto& seq, auto& task){
        task.downloadAssets.erase(uri);
        if (task.downloadAssets.size() == 0 && task.callback!= nullptr) {
            task.callback(result);
            tasks_.Erase(seq);
        }
        return true;
    });
}
ObjectAssetLoader::ObjectAssetLoader() {}
} // namespace OHOS::DistributedObject
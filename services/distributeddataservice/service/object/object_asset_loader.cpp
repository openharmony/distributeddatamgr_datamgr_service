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

bool ObjectAssetLoader::Transfer(const int32_t userId, const std::string& bundleName, const std::string& deviceId,
    const DistributedData::Asset& asset)
{
    AssetInfo assetInfo;
    assetInfo.uri = asset.uri;
    assetInfo.assetName = asset.name;
    ZLOGI("Start transfer, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
        DistributedData::Anonymous::Change(deviceId).c_str(), assetInfo.assetName.c_str());
    auto block = std::make_shared<BlockData<std::tuple<bool, int32_t>>>(WAIT_TIME, std::tuple{ true, OBJECT_SUCCESS });
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo,
        [block](const std::string& uri, int32_t status) {
            block->SetValue({ false, status });
        });
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
    ZLOGD("Transfer end, bundleName: %{public}s, deviceId: %{public}s, assetName: %{public}s", bundleName.c_str(),
        DistributedData::Anonymous::Change(deviceId).c_str(), assetInfo.assetName.c_str());
    return true;
}

void ObjectAssetLoader::TransferAssetsAsync(const int32_t userId, const std::string& bundleName,
    const std::string& deviceId, const std::vector<DistributedData::Asset>& assets, const TransferFunc& callback)
{
    if (executors_ == nullptr) {
        ZLOGE("executors is null, bundleName: %{public}s, deviceId: %{public}s, userId: %{public}d",
            bundleName.c_str(), DistributedData::Anonymous::Change(deviceId).c_str(), userId);
        callback(false);
        return;
    }
    TransferTask task;
    task.callback = callback;
    DistributedData::Assets downloadAssets;
    for (auto& asset : assets) {
        if (IsDownloaded(asset)) {
            continue;
        }
        task.downloadAssets.insert(asset.uri);
        downloadAssets.emplace_back(asset);
    }
    if (task.downloadAssets.empty()) {
        callback(true);
    }
    taskSeq_++;
    tasks_.ComputeIfAbsent(taskSeq_, [task](const uint32_t key) {
        return task;
    });
    executors_->Execute([this, userId, bundleName, deviceId, downloadAssets]() {
        bool result = true;
        for (const auto& asset : downloadAssets) {
            if (IsDownloaded(asset)) {
                FinishTask(asset.uri, result);
                continue;
            }
            if (IsDownloading(asset)) {
                continue;
            }
            downloading_.ComputeIfAbsent(asset.uri, [asset](const std::string& key) {
                return asset.hash;
            });
            auto success = Transfer(userId, bundleName, deviceId, asset);
            if (success) {
                std::lock_guard<std::mutex> lock(mutex_);
                downloading_.Erase(asset.uri);
                UpdateDownloaded(asset);
            } else {
                downloading_.Erase(asset.uri);
            }
            result &= success;
            FinishTask(asset.uri, result);
        }
    });
}

void ObjectAssetLoader::FinishTask(const std::string& uri, bool result)
{
    std::vector<uint32_t> finishedTasks;
    tasks_.ForEach([&uri, &finishedTasks, result](auto& seq, auto& task) {
        task.downloadAssets.erase(uri);
        if (task.downloadAssets.size() == 0 && task.callback != nullptr) {
            task.callback(result);
            finishedTasks.emplace_back(seq);
        }
        return false;
    });
    for (auto taskId : finishedTasks) {
        tasks_.Erase(taskId);
    }
}

void ObjectAssetLoader::UpdateDownloaded(const DistributedData::Asset& asset)
{
    downloaded_.ComputeIfAbsent(asset.uri, [asset](const std::string& key) {
        return asset.hash;
    });
    assetQueue_.push(asset.uri);
    if (assetQueue_.size() > LAST_DOWNLOAD_ASSET_SIZE) {
        auto oldAsset = assetQueue_.front();
        assetQueue_.pop();
        downloaded_.Erase(oldAsset);
    }
}

bool ObjectAssetLoader::IsDownloading(const DistributedData::Asset& asset)
{
    auto [success, hash] = downloading_.Find(asset.uri);
    if (success && hash == asset.hash) {
        ZLOGD("asset is downloading. assetName:%{public}s", asset.name.c_str());
        return true;
    }
    return false;
}

bool ObjectAssetLoader::IsDownloaded(const DistributedData::Asset& asset)
{
    auto [success, hash] = downloaded_.Find(asset.uri);
    if (success && hash == asset.hash) {
        ZLOGD("asset is downloaded. assetName:%{public}s", asset.name.c_str());
        return true;
    }
    return false;
}
} // namespace OHOS::DistributedObject
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
#include "cloud_sync_asset_manager.h"
#include "object_manager.h"
#include "log_print.h"

namespace OHOS::DistributedObject {
using namespace OHOS::FileManagement::CloudSync;
ObjectAssetLoader * ObjectAssetLoader::GetInstance()
{
    static ObjectAssetLoader *loader = new ObjectAssetLoader();
    return loader;
}

bool ObjectAssetLoader::DownLoad(const int32_t userId, const std::string &bundleName, 
    const std::string &deviceId, const ObjectStore::Asset &assetValue)
{
    bool result = false;
    bool complete = false;

    AssetInfo assetInfo;
    assetInfo.uri = assetValue.uri;
    assetInfo.assetName = assetValue.name;
    ZLOGD("start download file userId: %{public}d, bundleName: %{public}s, networkId: %{public}s, \
        asset name : %{public}s", userId, bundleName.c_str(), deviceId.c_str(), assetValue.name.c_str());
    auto res = CloudSyncAssetManager::GetInstance().DownloadFile(userId, bundleName, deviceId, assetInfo, 
        [&](const std::string &uri, int32_t status) {
            std::unique_lock<std::mutex> lock(mutex_);
            complete = true;
            if (status == OBJECT_SUCCESS) {
                result = true;
            }
            ZLOGD("DownloadFile callback status: %{public}d", status);
            cv.notify_one();
        });
    if (res != OBJECT_SUCCESS) {
        ZLOGE("DownloadFile fail, status: %{public}d, bundleName: %{public}s, asset name : %{public}s", 
            res, bundleName.c_str(), assetValue.name.c_str());
        return false;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    cv.wait_for(lock, std::chrono::seconds(WAIT_TIME), [&complete]() {
        return complete;
    });
    if(!complete){
        ZLOGE("DownloadFile time out, status: %{public}d, bundleName: %{public}s, asset name : %{public}s", 
            res, bundleName.c_str(), assetValue.name.c_str());
    }
    return result;
}
}
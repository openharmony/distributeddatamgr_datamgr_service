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
#ifndef DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H
#define DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H

#include <string>
#include "executor_pool.h"
#include "object_types.h"
#include "store/general_value.h"
#include "concurrent_map.h"
#include <unordered_set>
namespace OHOS {
namespace DistributedObject {

using TransferFunc = std::function<void(bool success)>;
struct TransferTask {
    TransferTask() = default;
    std::unordered_set<std::string> downloadAssets;
    TransferFunc callback;
};

class ObjectAssetLoader {
public:
    static ObjectAssetLoader *GetInstance();
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    bool Transfer(const int32_t userId, const std::string& bundleName, const std::string& deviceId,
        const DistributedData::Asset& asset);
    void TransferAssetsAsync(const int32_t userId, const std::string& bundleName, const std::string& deviceId,
                             const std::vector<DistributedData::Asset>& assets, const TransferFunc& callback);
private:
    ObjectAssetLoader() = default;
    ~ObjectAssetLoader() = default;
    ObjectAssetLoader(const ObjectAssetLoader &) = delete;
    ObjectAssetLoader &operator=(const ObjectAssetLoader &) = delete;
    void FinishTask(const std::string& uri, bool result);
    bool IsDownloading(const DistributedData::Asset& asset);
    bool IsDownloaded(const DistributedData::Asset& asset);
    void UpdateDownloaded(const DistributedData::Asset& asset);
    static constexpr int WAIT_TIME = 60;
    static constexpr int LAST_DOWNLOAD_ASSET_SIZE = 200;
    std::shared_ptr<ExecutorPool> executors_;
    std::mutex mutex_;
    std::atomic_uint32_t taskSeq_ = 0;
    std::queue<std::string> assetQueue_;
    ConcurrentMap<uint32_t, TransferTask> tasks_;
    ConcurrentMap<std::string, std::string> downloaded_;
    ConcurrentMap<std::string, std::string> downloading_;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_ASSET_LOADER_H
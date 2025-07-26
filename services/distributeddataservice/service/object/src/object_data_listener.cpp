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

#define LOG_TAG "ObjectDataListener"

#include "object_data_listener.h"
#include "log_print.h"
#include "object_manager.h"
#include "object_radar_reporter.h"
#include "utils/anonymous.h"
namespace OHOS {
namespace DistributedObject {
constexpr int32_t PROGRESS_MAX = 100;
constexpr int32_t PROGRESS_INVALID = -1;
ObjectDataListener::ObjectDataListener()
{
}

ObjectDataListener::~ObjectDataListener()
{
}

void ObjectDataListener::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    const auto &insertedDatas = data.GetEntriesInserted();
    const auto &updatedDatas = data.GetEntriesUpdated();
    std::map<std::string, std::vector<uint8_t>> changedData {};
    for (const auto &entry : insertedDatas) {
        std::string key(entry.key.begin(), entry.key.end());
        changedData.insert_or_assign(std::move(key), entry.value);
    }
    for (const auto &entry : updatedDatas) {
        std::string key(entry.key.begin(), entry.key.end());
        changedData.insert_or_assign(std::move(key), entry.value);
    }
    DistributedObject::ObjectStoreManager::GetInstance().NotifyChange(changedData);
}

int32_t ObjectAssetsRecvListener::OnStart(const std::string &srcNetworkId, const std::string &dstNetworkId,
    const std::string &sessionId, const std::string &dstBundleName)
{
    auto objectKey = dstBundleName + sessionId;
    ZLOGI("OnStart, objectKey:%{public}s", DistributedData::Anonymous::Change(objectKey).c_str());
    ObjectStoreManager::GetInstance().NotifyAssetsStart(objectKey, srcNetworkId);
    ObjectStoreManager::GetInstance().NotifyAssetsRecvProgress(objectKey, 0);
    return OBJECT_SUCCESS;
}

int32_t ObjectAssetsRecvListener::OnFinished(const std::string &srcNetworkId, const sptr<AssetObj> &assetObj,
    int32_t result)
{
    if (assetObj == nullptr) {
        ZLOGE("OnFinished error! status:%{public}d, srcNetworkId:%{public}s", result,
            DistributedData::Anonymous::Change(srcNetworkId).c_str());
        ObjectStore::RadarReporter::ReportStageError(std::string(__FUNCTION__), ObjectStore::DATA_RESTORE,
            ObjectStore::ASSETS_RECV, ObjectStore::RADAR_FAILED, result);
        return result;
    }
    auto objectKey = assetObj->dstBundleName_ + assetObj->sessionId_;
    ZLOGI("OnFinished, status:%{public}d objectKey:%{public}s, asset size:%{public}zu", result,
        DistributedData::Anonymous::Change(objectKey).c_str(), assetObj->uris_.size());
    ObjectStoreManager::GetInstance().NotifyAssetsReady(objectKey, assetObj->dstBundleName_, srcNetworkId);
    if (result != OBJECT_SUCCESS) {
        ObjectStoreManager::GetInstance().NotifyAssetsRecvProgress(objectKey, PROGRESS_INVALID);
    } else {
        ObjectStoreManager::GetInstance().NotifyAssetsRecvProgress(objectKey, PROGRESS_MAX);
    }
    return OBJECT_SUCCESS;
}

int32_t ObjectAssetsRecvListener::OnRecvProgress(
    const std::string &srcNetworkId, const sptr<AssetObj> &assetObj, uint64_t totalBytes, uint64_t processBytes)
{
    if (assetObj == nullptr || totalBytes == 0) {
        ZLOGE("OnRecvProgress error! srcNetworkId:%{public}s, totalBytes: %{public}llu",
            DistributedData::Anonymous::Change(srcNetworkId).c_str(), totalBytes);
        return OBJECT_INNER_ERROR;
    }

    auto objectKey = assetObj->dstBundleName_ + assetObj->sessionId_;
    ZLOGI("srcNetworkId: %{public}s, objectKey:%{public}s, totalBytes: %{public}llu,"
          "processBytes: %{public}llu.",
        DistributedData::Anonymous::Change(srcNetworkId).c_str(),
        DistributedData::Anonymous::Change(objectKey).c_str(), totalBytes, processBytes);

    int32_t progress = static_cast<int32_t>((processBytes * 100.0 / totalBytes) * 0.9);
    ObjectStoreManager::GetInstance().NotifyAssetsRecvProgress(objectKey, progress);
    return OBJECT_SUCCESS;
}
}  // namespace DistributedObject
}  // namespace OHOS

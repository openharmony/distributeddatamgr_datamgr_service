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
    DistributedObject::ObjectStoreManager::GetInstance()->NotifyChange(changedData);
}

int32_t ObjectAssetsRecvListener::OnStart(const std::string &srcNetworkId, const std::string &dstNetworkId,
    const std::string &sessionId, const std::string &dstBundleName)
{
    auto objectKey = dstBundleName + sessionId;
    ZLOGI("OnStart, objectKey:%{public}s", objectKey.c_str());
    ObjectStoreManager::GetInstance()->NotifyAssetsStart(objectKey, srcNetworkId);
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
    auto objectKey = assetObj->dstBundleName_+assetObj->sessionId_;
    ZLOGI("OnFinished, status:%{public}d objectKey:%{public}s, asset size:%{public}zu", result, objectKey.c_str(),
        assetObj->uris_.size());
    ObjectStoreManager::GetInstance()->NotifyAssetsReady(objectKey, assetObj->dstBundleName_, srcNetworkId);
    return OBJECT_SUCCESS;
}
}  // namespace DistributedObject
}  // namespace OHOS

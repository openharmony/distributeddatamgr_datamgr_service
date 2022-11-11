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

#define LOG_TAG "CalcSyncDataSizeImpl"
#include "calc_kvdb_sync_data_size.h"
#include "calc_sync_data_size_impl.h"
#include "communication_provider.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "kvstore_meta_manager.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data_local.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
using namespace OHOS::AppDistributedKv;
using Commu = AppDistributedKv::CommunicationProvider;
using KvStoreUtils = OHOS::DistributedKv::KvStoreUtils;
using DMAdapter = DistributedData::DeviceManagerAdapter;
CalcSyncDataSizeImpl::CalcSyncDataSizeImpl()
{
    Commu::GetInstance().StartWatchDeviceChange(this, { "calcSyncDataSize" });
}

uint32_t CalcSyncDataSizeImpl::CalcDataSize(const std::string &deviceId)
{
    auto it = dataSizes_.Find(deviceId);
    if (!it.first) {
        return 0;
    }
    return it.second;
}

void CalcSyncDataSizeImpl::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
    const AppDistributedKv::DeviceChangeType &type) const
{
    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            CalcSyncDataSize(info.uuid);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONREADY:
            dataSizes_.Erase(info.uuid);
            break;
        default:
            break;
    }
}

void CalcSyncDataSizeImpl::CalcSyncDataSize(const std::string &deviceId) const
{
    uint32_t metaSize = CalcMetaDataSize(deviceId);
    uint32_t dataSize = CalcKvDataSize(deviceId);
    uint32_t totalSize = metaSize + dataSize;
    dataSizes_.InsertOrAssign(deviceId, totalSize);
    ZLOGI("deviceId: %{public}s, sync total size:%{public}u, meta:%{public}u data:%{public}u",
          KvStoreUtils::ToBeAnonymous(deviceId).c_str(), totalSize, metaSize, dataSize);
    return;
}

uint32_t CalcSyncDataSizeImpl::CalcMetaDataSize(const std::string &deviceId) const
{
    uint32_t dataSize = 0;
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    store->CalculateSyncDataSize(deviceId, dataSize);
    return dataSize;
}

uint32_t CalcSyncDataSizeImpl::CalcKvDataSize(const std::string &deviceId) const
{
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("load meta failed!");
        return 0;
    }

    uint32_t totalSize = 0;
    for (const auto &data : metaData) {
        StoreMetaDataLocal localMetaData;
        MetaDataManager::GetInstance().LoadMeta(data.GetKeyLocal(), localMetaData, true);
        if (!localMetaData.HasPolicy(PolicyType::IMMEDIATE_SYNC_ON_ONLINE)) {
            continue;
        }

        DistributedDB::DBStatus status;
        uint32_t dataSize = CalcKvSyncDataSize::CalcSyncDataSize(data, status);
        totalSize += dataSize;
    }
    return totalSize;
}
}
/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "RdbDataSyncReporter"

#include "rdb_data_sync_reporter.h"
#include "communicator/device_manager_adapter.h"
#include "commonevent/data_sync_event.h"
#include "error/general_error.h"
#include "eventcenter/event_center.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {
using namespace DistributedData;
RdbDataSyncReporter::RdbDataSyncReporter(const StoreInfo &storeInfo)
    : storeInfo_(storeInfo)
{
}

RdbDataSyncReporter::~RdbDataSyncReporter()
{
    if (syncModeInfo_.Size() == 0) {
        return;
    }
    syncModeInfo_.ForEach([storeInfo = storeInfo_](const uint32_t &key, int32_t &value) {
        StoreInfo info = storeInfo;
        auto evt = std::make_unique<DataSyncEvent>(std::move(info), key, DataSyncEvent::DataSyncStatus::FINISH);
        EventCenter::GetInstance().PostEvent(std::move(evt));
        return false;
    });
}

void RdbDataSyncReporter::OnSyncStart(uint32_t syncMode, int status)
{
    if (status != GeneralError::E_OK) {
        return;
    }
    bool postEvent = false;
    syncModeInfo_.Compute(syncMode, [&postEvent](const uint32_t &key, int32_t &value) {
        if (value == 0) {
            postEvent = true;
        }
        value++;
        return true;
    });
    if (postEvent) {
        StoreInfo info = storeInfo_;
        auto evt = std::make_unique<DataSyncEvent>(std::move(info), syncMode, DataSyncEvent::DataSyncStatus::START);
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
}

void RdbDataSyncReporter::OnSyncFinish(uint32_t syncMode)
{
    bool postEvent = false;
    syncModeInfo_.EraseIf([syncMode, &postEvent](const uint32_t &key, int32_t &value) {
        if (key != syncMode) {
            return false;
        }
        value--;
        if (value == 0) {
            postEvent = true;
        }
        return value == 0;
    });
    if (postEvent) {
        StoreInfo info = storeInfo_;
        auto evt = std::make_unique<DataSyncEvent>(std::move(info), syncMode, DataSyncEvent::DataSyncStatus::FINISH);
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
}
} // OHOS::DistributedRdb
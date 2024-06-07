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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_DATA_SYNC_REPORTER_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_DATA_SYNC_REPORTER_H

#include "concurrent_map.h"
#include "store/store_info.h"

namespace OHOS::DistributedRdb {
class RdbDataSyncReporter {
public:
    explicit RdbDataSyncReporter(const DistributedData::StoreInfo &storeInfo);
    ~RdbDataSyncReporter();
    void OnSyncStart(uint32_t syncMode, int status);
    void OnSyncFinish(uint32_t syncMode);

private:
    DistributedData::StoreInfo storeInfo_;
    // syncMode, count
    ConcurrentMap<uint32_t, int32_t> syncModeInfo_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_DATA_SYNC_REPORTER_H

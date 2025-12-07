/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DATAMGR_SERVICE_RDB_HIVIEW_ADAPTER_H
#define DATAMGR_SERVICE_RDB_HIVIEW_ADAPTER_H

#include "concurrent_map.h"
#include "executor_pool.h"
#include "hisysevent.h"
#include "rdb_types.h"

namespace OHOS::DistributedRdb {
enum SetDeviceDisTableErrorCode : int32_t {
    SETDEVICETABLE_SUCCESS = 0,
    SETDEVICETABLE_NOSCHEMA = 1,
    SETDEVICETABLE_SETCONFIG_FAIL,
    SETDEVICETABLE_SUNC_FIELD_IS_AUTOINCREMENT,
    SETDEVICETABLE_SCHEMA_PRIMARYKEY_COUNT_IS_WRONG,
    SETDEVICETABLE_SETSCHEMA_FAIL,
    DEVICE_SYNC_LIMIT,
};
constexpr int MAX_TIME_BUF_LEN = 32;
constexpr int MILLISECONDS_LEN = 3;
constexpr int NANO_TO_MILLI = 1000000;
constexpr int MILLI_PRE_SEC = 1000;
constexpr const char *SET_DEVICE_DIS_TABLE = "SET_RDB_DEVICE_DISTRIBUTED_TABLE";
constexpr const char *DEVICE_SYNC = "DEVICE_SYNC";

struct RdbFaultEvent {
    std::string faultType;
    int32_t errorCode;
    std::string bundleName;
    std::string custLog;
};

class RdbHiViewAdapter {
public:
    static RdbHiViewAdapter &GetInstance();
    void ReportStatistic(const RdbStatEvent &stat);
    void ReportRdbFault(const RdbFaultEvent &stat);
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);

private:
    std::shared_ptr<ExecutorPool> executors_ = nullptr;
    ConcurrentMap<RdbStatEvent, uint32_t> statEvents_;
    void InvokeSync();
    void StartTimerThread();
    std::atomic<bool> running_ = false;
};
} // namespace OHOS::DistributedRdb
#endif // DATAMGR_SERVICE_RDB_HIVIEW_ADAPTER_H
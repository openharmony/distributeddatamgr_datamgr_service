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

class RdbHiViewAdapter {
public:
  static RdbHiViewAdapter &GetInstance();
  void ReportStatistic(const RdbStatEvent &stat);
  void SetThreadPool(std::shared_ptr<ExecutorPool> executors);

private:
  std::shared_ptr<ExecutorPool> executors_ = nullptr;
  ConcurrentMap<RdbStatEvent, uint32_t> statEventMap;
  void InvokeSync();
  void StartTimerThread();

private:
  std::atomic<bool> running_ = false;
  const int waitTime = 60 * 60 * 24; // 24h
};
} // namespace OHOS::DistributedRdb
#endif // DATAMGR_SERVICE_RDB_HIVIEW_ADAPTER_H
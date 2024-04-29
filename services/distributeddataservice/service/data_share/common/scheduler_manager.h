/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SCHEDULER_MANAGER_H
#define SCHEDULER_MANAGER_H

#include <memory>

#include "db_delegate.h"
#include "executor_pool.h"
#include "subscriber_managers/rdb_subscriber_manager.h"

namespace OHOS::DataShare {
class SchedulerManager {
public:
    static SchedulerManager &GetInstance();
    void Execute(const std::string &uri, const int32_t userId, const std::string &rdbDir, int version);
    void Execute(const Key &key, const int32_t userId, const std::string &rdbDir, int version);
    void ReExecuteAll();
    void SetTimer(const std::string &dbPath, const int32_t userId, int version, const Key &key, int64_t reminderTime);
    void RemoveTimer(const Key &key);
    void ClearTimer();
    void SetExecutorPool(std::shared_ptr<ExecutorPool> executor);

private:
    static constexpr const char *REMIND_TIMER_FUNC = "remindTimer(";
    static constexpr int REMIND_TIMER_FUNC_LEN = 12;
    SchedulerManager() = default;
    ~SchedulerManager() = default;
    static void GenRemindTimerFuncParams(const std::string &rdbDir, const int32_t userId, int version, const Key &key,
        std::string &schedulerSQL);
    void ExecuteSchedulerSQL(const std::string &rdbDir, const int32_t userId, int version, const Key &key,
        std::shared_ptr<DBDelegate> delegate);
    bool SetTimerTask(uint64_t &timerId, const std::function<void()> &callback, int64_t reminderTime);
    void DestoryTimerTask(int64_t timerId);
    void ResetTimerTask(int64_t timerId, int64_t reminderTime);

    std::mutex mutex_;
    std::map<Key, int64_t> timerCache_;
    std::shared_ptr<ExecutorPool> executor_ = nullptr;
};
} // namespace OHOS::DataShare
#endif // SCHEDULER_MANAGER_H

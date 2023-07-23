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
#define LOG_TAG "SchedulerManager"

#include "scheduler_manager.h"

#include "log_print.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
SchedulerManager &SchedulerManager::GetInstance()
{
    static SchedulerManager instance;
    return instance;
}

void SchedulerManager::Execute(const std::string &uri, const int32_t userId, const std::string &rdbDir, int version)
{
    if (!URIUtils::IsDataProxyURI(uri)) {
        return;
    }
    auto delegate = DBDelegate::Create(rdbDir, version, true);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return;
    }
    std::vector<Key> keys = RdbSubscriberManager::GetInstance().GetKeysByUri(uri);
    for (auto const &key : keys) {
        ExecuteSchedulerSQL(rdbDir, userId, version, key, delegate);
    }
}

void SchedulerManager::Execute(const Key &key, const int32_t userId, const std::string &rdbDir, int version)
{
    auto delegate = DBDelegate::Create(rdbDir, version, true);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s", DistributedData::Anonymous::Change(key.uri).c_str());
        return;
    }
    ExecuteSchedulerSQL(rdbDir, userId, version, key, delegate);
}

void SchedulerManager::SetTimer(
    const std::string &dbPath, const int32_t userId, int version, const Key &key, int64_t reminderTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (executor_ == nullptr) {
        ZLOGE("executor_ is nullptr");
        return;
    }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    // reminder time must is in future
    if (reminderTime <= now) {
        ZLOGE("reminderTime is not in future, %{public}" PRId64 "%{public}" PRId64, reminderTime, now);
        return;
    }
    auto duration = std::chrono::milliseconds(reminderTime - now);
    ZLOGI("reminderTime will notify in %{public}" PRId64 " milliseconds", reminderTime - now);
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        // has current timer, reset time
        ZLOGD("has current taskId, uri is %{private}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
            key.uri.c_str(), key.subscriberId, key.bundleName.c_str());
        executor_->Reset(it->second, duration);
        return;
    }
    // not find task in map, create new timer
    auto taskId = executor_->Schedule(duration, [key, dbPath, version, userId, this]() {
        ZLOGI("schedule notify start, uri is %{private}s, subscriberId is %{public}" PRId64 ", bundleName is "
            "%{public}s", key.uri.c_str(), key.subscriberId, key.bundleName.c_str());
        timerCache_.erase(key);
        // 1. execute schedulerSQL in next time
        Execute(key, userId, dbPath, version);
        // 2. notify
        RdbSubscriberManager::GetInstance().EmitByKey(key, userId, dbPath, version);
    });
    if (taskId == ExecutorPool::INVALID_TASK_ID) {
        ZLOGE("create timer failed, over the max capacity");
        return;
    }
    ZLOGI("create new task success, uri is %{public}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
        DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
    timerCache_.emplace(key, taskId);
}

void SchedulerManager::ExecuteSchedulerSQL(const std::string &rdbDir, const int32_t userId, int version, const Key &key,
    std::shared_ptr<DBDelegate> delegate)
{
    Template tpl;
    if (!TemplateManager::GetInstance().Get(key, userId, tpl)) {
        ZLOGE("template undefined, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return;
    }
    if (tpl.scheduler_.empty()) {
        ZLOGW("template scheduler_ empty, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return;
    }
    GenRemindTimerFuncParams(rdbDir, userId, version, key, tpl.scheduler_);
    auto resultSet = delegate->QuerySql(tpl.scheduler_);
    if (resultSet == nullptr) {
        ZLOGE("resultSet is nullptr, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return;
    }
    int count;
    int errCode = resultSet->GetRowCount(count);
    if (errCode != E_OK || count == 0) {
        ZLOGE("GetRowCount error, %{public}s, %{public}" PRId64 ", %{public}s, errorCode is %{public}d, count is "
            "%{public}d",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str(), errCode,
            count);
        return;
    }
    errCode = resultSet->GoToFirstRow();
    if (errCode != E_OK) {
        ZLOGE("GoToFirstRow error, %{public}s, %{public}" PRId64 ", %{public}s, errCode is %{public}d",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str(), errCode);
    }
}

void SchedulerManager::GenRemindTimerFuncParams(
    const std::string &rdbDir, const int32_t userId, int version, const Key &key, std::string &schedulerSQL)
{
    auto index = schedulerSQL.find(REMIND_TIMER_FUNC);
    if (index == std::string::npos) {
        ZLOGW("not find remindTimer, sql is %{public}s", schedulerSQL.c_str());
        return;
    }
    index += REMIND_TIMER_FUNC_LEN;
    std::string keyStr = "'" + rdbDir + "', " + std::to_string(version) + ", '" + key.uri + "', " +
                         std::to_string(key.subscriberId) + ", '" + key.bundleName + "', " + std::to_string(userId) +
                         ", ";
    schedulerSQL.insert(index, keyStr);
    return;
}

void SchedulerManager::RemoveTimer(const Key &key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (executor_ == nullptr) {
        ZLOGE("executor_ is nullptr");
        return;
    }
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        ZLOGW("RemoveTimer %{public}s %{public}s %{public}" PRId64,
            DistributedData::Anonymous::Change(key.uri).c_str(), key.bundleName.c_str(), key.subscriberId);
        executor_->Remove(it->second);
        timerCache_.erase(key);
    }
}

void SchedulerManager::ClearTimer()
{
    ZLOGI("Clear all timer");
    std::lock_guard<std::mutex> lock(mutex_);
    if (executor_ == nullptr) {
        ZLOGE("executor_ is nullptr");
        return;
    }
    auto it = timerCache_.begin();
    while (it != timerCache_.end()) {
        executor_->Remove(it->second);
        it = timerCache_.erase(it);
    }
}

void SchedulerManager::SetExecutorPool(std::shared_ptr<ExecutorPool> executor)
{
    executor_ = executor;
}

void SchedulerManager::ReExecuteAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &item : timerCache_) {
        // restart in 200ms
        executor_->Reset(item.second, std::chrono::milliseconds(200));
    }
}
} // namespace OHOS::DataShare


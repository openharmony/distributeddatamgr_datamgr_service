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

void SchedulerManager::Execute(const std::string &uri, const std::string &rdbDir, int version)
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
    for (auto &key : keys) {
        if (RdbSubscriberManager::GetInstance().GetObserverCount(key) == 0) {
            continue;
        }
        ExecuteSchedulerSQL(rdbDir, version, key, delegate);
    }
}

void SchedulerManager::Execute(const Key &key, const std::string &rdbDir, int version)
{
    auto delegate = DBDelegate::Create(rdbDir, version, true);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s", DistributedData::Anonymous::Change(key.uri).c_str());
        return;
    }
    ExecuteSchedulerSQL(rdbDir, version, key, delegate);
}

void SchedulerManager::SetTimer(const std::string &dbPath, int version, const Key &key, int64_t reminderTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (scheduler_ == nullptr) {
        scheduler_ = std::make_shared<TaskScheduler>(TIME_TASK_NUM, "remind_timer");
    }
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        // has current timer, reset time
        ZLOGD("has current taskId, uri is %{public}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
        DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        // scheduler_->Reset(it->second, std::chrono::seconds(reminderTime - time(nullptr)));
        scheduler_->Reset(it->second, std::chrono::seconds(7));
        return;
    }
    // not find task in map, create new timer
    // auto taskId = scheduler_->At(TaskScheduler::Clock::now() + std::chrono::seconds(reminderTime - time(nullptr)),
    auto taskId = scheduler_->At(TaskScheduler::Clock::now() + std::chrono::seconds(7),
        [key, dbPath, version, this]() {
            timerCache_.erase(key);
            // 1. execute schedulerSQL in next time
            Execute(key, dbPath, version);
            // 2. notify
            RdbSubscriberManager::GetInstance().EmitByKey(key, dbPath, version);
        });
    if (taskId == TaskScheduler::INVALID_TASK_ID) {
        ZLOGE("create timer failed, over the max capacity");
        return;
    }
    ZLOGI("create new task success, uri is %{public}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
        DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
    timerCache_.emplace(key, taskId);
}

void SchedulerManager::ExecuteSchedulerSQL(const std::string &rdbDir, int version, const Key &key,
    std::shared_ptr<DBDelegate> delegate)
{
    Template tpl;
    if (!TemplateManager::GetInstance().GetTemplate(key.uri, key.subscriberId, key.bundleName, tpl)) {
        ZLOGE("template undefined, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return;
    }
    if (tpl.scheduler_.empty()) {
        ZLOGW("template scheduler_ empty, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        return;
    }
    GenRemindTimerFuncParams(rdbDir, version, key, tpl.scheduler_);
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

void SchedulerManager::GenRemindTimerFuncParams(const std::string &rdbDir, int version, const Key &key,
    std::string &schedulerSQL)
{
    auto index = schedulerSQL.find(REMIND_TIMER_FUNC);
    if (index == -1) {
        ZLOGW("not find remindTimer, sql is %{public}s", schedulerSQL.c_str());
        return;
    }
    index += REMIND_TIMER_FUNC_LEN;
    std::string keyStr = "'" + rdbDir + "', " + std::to_string(version) + ", '" + key.uri + "', " +
                         std::to_string(key.subscriberId) + ", '" + key.bundleName + "', ";
    schedulerSQL.insert(index, keyStr);
    return;
}

void SchedulerManager::RemoveTimer(const Key &key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (scheduler_ == nullptr) {
        ZLOGD("scheduler_ is nullptr");
        return;
    }
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        ZLOGD("RemoveTimer %{public}s %{public}s %{public}" PRId64,
            DistributedData::Anonymous::Change(key.uri).c_str(), key.bundleName.c_str(), key.subscriberId);
        scheduler_->Remove(it->second);
        timerCache_.erase(key);
        if (timerCache_.empty()) {
            scheduler_ = nullptr;
        }
    }
}
} // namespace OHOS::DataShare


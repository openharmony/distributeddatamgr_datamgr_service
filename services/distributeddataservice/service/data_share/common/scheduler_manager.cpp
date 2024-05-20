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

#include <cinttypes>

#include "log_print.h"
#include "timer_info.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
static constexpr int64_t MAX_MILLISECONDS = 31536000000; // 365 days
static constexpr int32_t DELAYED_MILLISECONDS = 200;
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

bool SchedulerManager::SetTimerTask(uint64_t &timerId, const std::function<void()> &callback,
    int64_t reminderTime)
{
    auto timerInfo = std::make_shared<TimerInfo>();
    timerInfo->SetType(timerInfo->TIMER_TYPE_EXACT);
    timerInfo->SetRepeat(false);
    auto wantAgent = std::shared_ptr<AbilityRuntime::WantAgent::WantAgent>();
    timerInfo->SetWantAgent(wantAgent);
    timerInfo->SetCallbackInfo(callback);
    timerId = TimeServiceClient::GetInstance()->CreateTimer(timerInfo);
    if (timerId == 0) {
        return false;
    }
    TimeServiceClient::GetInstance()->StartTimer(timerId, static_cast<uint64_t>(reminderTime));
    return true;
}

void SchedulerManager::DestoryTimerTask(int64_t timerId)
{
    if (timerId > 0) {
        TimeServiceClient::GetInstance()->DestroyTimer(timerId);
    }
}

void SchedulerManager::ResetTimerTask(int64_t timerId, int64_t reminderTime)
{
    TimeServiceClient::GetInstance()->StopTimer(timerId);
    TimeServiceClient::GetInstance()->StartTimer(timerId, static_cast<uint64_t>(reminderTime));
}

void SchedulerManager::SetTimer(
    const std::string &dbPath, const int32_t userId, int version, const Key &key, int64_t reminderTime)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (executor_ == nullptr) {
        ZLOGE("executor_ is nullptr");
        return;
    }
    int64_t now = 0;
    TimeServiceClient::GetInstance()->GetWallTimeMs(now);
    if (reminderTime <= now || reminderTime - now >= MAX_MILLISECONDS) {
        ZLOGE("invalid args, %{public}" PRId64 ", %{public}" PRId64 ", subId=%{public}" PRId64
            ", bundleName=%{public}s.", reminderTime, now, key.subscriberId, key.bundleName.c_str());
        return;
    }
    auto duration = reminderTime - now;
    ZLOGI("the task will notify in %{public}" PRId64 " ms, %{public}" PRId64 ", %{public}s.",
          duration, key.subscriberId, key.bundleName.c_str());
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        ZLOGD("has current taskId, uri is %{private}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
        auto timerId = it->second;
        ResetTimerTask(timerId, reminderTime);
        return;
    }
    auto callback = [key, dbPath, version, userId, this]() {
        ZLOGI("schedule notify start, uri is %{private}s, subscriberId is %{public}" PRId64 ", bundleName is "
            "%{public}s", DistributedData::Anonymous::Change(key.uri).c_str(),
            key.subscriberId, key.bundleName.c_str());
        int64_t timerId = -1;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = timerCache_.find(key);
            if (it != timerCache_.end()) {
                timerId = it->second;
                timerCache_.erase(key);
            }
        }
        DestoryTimerTask(timerId);
        Execute(key, userId, dbPath, version);
        RdbSubscriberManager::GetInstance().EmitByKey(key, userId, dbPath, version);
    };
    uint64_t timerId = 0;
    if (!SetTimerTask(timerId, callback, reminderTime)) {
        ZLOGE("create timer failed.");
        return;
    }
    ZLOGI("create new task success, uri is %{public}s, subscriberId is %{public}" PRId64 ", bundleName is %{public}s",
        DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
    timerCache_.emplace(key, timerId);
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
        DestoryTimerTask(it->second);
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
        DestoryTimerTask(it->second);
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
    for (const auto &item : timerCache_) {
        // restart in 200ms
        auto timerId = item.second;
        int64_t currentTime = 0;
        TimeServiceClient::GetInstance()->GetWallTimeMs(currentTime);
        ResetTimerTask(timerId, currentTime + DELAYED_MILLISECONDS);
    }
}
} // namespace OHOS::DataShare


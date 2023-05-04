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
    int errCode = 0;
    auto delegate = DBDelegate::Create(rdbDir, version, errCode, true);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return;
    }
    std::vector<Key> keys = RdbSubscriberManager::GetInstance().GetKeysByUri(uri);
    for (auto &key : keys) {
        if (RdbSubscriberManager::GetInstance().GetObserverCount(key) == 0) {
            continue;
        }
        ExecuteSchedulerSQL(key, delegate);
    }
}

void SchedulerManager::Execute(const Key &key, const std::string &rdbDir, int version)
{
    int errCode = 0;
    auto delegate = DBDelegate::Create(rdbDir, version, errCode, true);
    if (delegate == nullptr) {
        ZLOGE("malloc fail %{public}s", DistributedData::Anonymous::Change(key.uri).c_str());
        return;
    }
    ExecuteSchedulerSQL(key, delegate);
}

void SchedulerManager::SetTimer(const RdbStoreContext &rdbContext, const std::string &uri, int64_t subscriberId,
    const std::string &bundleName, int64_t reminderTime)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (scheduler_ == nullptr) {
        scheduler_ = std::make_shared<TaskScheduler>(TIME_TASK_NUM, "remind_timer");
    }
    auto manager = RdbSubscriberManager::GetInstance();
    Key key(uri, subscriberId, bundleName);
    auto it = timerCache_.find(key);
    if (it != timerCache_.end()) {
        // has current timer, reset time
        std::time_t now = time(nullptr);
        scheduler_->Reset(it->second, std::chrono::seconds(reminderTime - now));
        return;
    }
    // not find task in map, create new timer
    std::time_t now = time(nullptr);
    auto taskId = scheduler_->At(TaskScheduler::Clock::now() + std::chrono::seconds(reminderTime - now), [&]() {
        // 1. execute schedulerSQL in next time
        Execute(key, rdbContext.dir, rdbContext.version);
        // 2. notify
        RdbSubscriberManager::GetInstance().EmitByKey(key, rdbContext.dir, rdbContext.version);
    });
    if (taskId == TaskScheduler::INVALID_TASK_ID) {
        ZLOGE("create timer failed, over the max capacity");
        return;
    }
    timerCache_.emplace(key, taskId);
}

void SchedulerManager::ExecuteSchedulerSQL(const Key &key, std::shared_ptr<DBDelegate> delegate)
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

    if (!GenRemindTimerFuncParams(key.uri, key.subscriberId, key.bundleName, tpl.scheduler_)) {
        return;
    }
    int errCode = delegate->ExecuteSql(tpl.scheduler_);
    if (errCode != E_OK) {
        ZLOGE("Execute schedulerSql failed, %{public}s, %{public}" PRId64 ", %{public}s",
            DistributedData::Anonymous::Change(key.uri).c_str(), key.subscriberId, key.bundleName.c_str());
    }
}

bool SchedulerManager::GenRemindTimerFuncParams(const std::string &uri, int64_t subscriberId,
    const std::string &bundleName, std::string &schedulerSQL)
{
    auto index = schedulerSQL.find("remindTimer(");
    if (index == -1) {
        ZLOGE("not find remindTimer, sql is %{public}s", schedulerSQL.c_str());
        return false;
    }
    index += strlen("remindTimer(");
    std::string keyStr = "\"" + uri + "\", " + std::to_string(subscriberId) + ", \"" + bundleName + "\", ";
    schedulerSQL.insert(index, keyStr);
    return true;
}

void SchedulerManager::RemoveTimer(const Key &key)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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

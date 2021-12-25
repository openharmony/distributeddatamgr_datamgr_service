/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "single_ver_sync_engine.h"
#include "db_common.h"
#include "single_ver_sync_task_context.h"
#include "log_print.h"

namespace DistributedDB {
ISyncTaskContext *SingleVerSyncEngine::CreateSyncTaskContext()
{
    auto context = new (std::nothrow) SingleVerSyncTaskContext();
    if (context == nullptr) {
        LOGE("[SingleVerSyncEngine][CreateSyncTaskContext] create failed, may be out of memory");
        return nullptr;
    }
    context->SetSyncRetry(GetSyncRetry());
    context->EnableClearRemoteStaleData(needClearRemoteStaleData_);
    context->SetSubscribeManager(subManager_);
    return context;
}

void SingleVerSyncEngine::EnableClearRemoteStaleData(bool enable)
{
    LOGI("[SingleVerSyncEngine][EnableClearRemoteStaleData] enabled %d", enable);
    needClearRemoteStaleData_ = enable;
    std::unique_lock<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        auto context = static_cast<SingleVerSyncTaskContext *>(iter.second);
        if (context != nullptr) {
            context->EnableClearRemoteStaleData(enable);
        }
    }
}

int SingleVerSyncEngine::StartAutoSubscribeTimer()
{
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    if (subscribeTimerId_ > 0) {
        LOGI("[SingleSyncEngine] subscribeTimerId is already set");
        return -E_INTERNAL_ERROR;
    }
    TimerId timerId = 0;
    TimerAction timeOutCallback = std::bind(&SingleVerSyncEngine::SubscribeTimeOut, this, std::placeholders::_1);
    int errCode = RuntimeContext::GetInstance()->SetTimer(SUBSCRIBE_TRIGGER_TIME_OUT, timeOutCallback, nullptr,
        timerId);
    if (errCode != E_OK) {
        return errCode;
    }
    subscribeTimerId_ = timerId;
    LOGI("[SingleSyncEngine] start auto subscribe timerId=%d finished", timerId);
    return errCode;
}

void SingleVerSyncEngine::StopAutoSubscribeTimer()
{
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    if (subscribeTimerId_ == 0) {
        return;
    }
    LOGI("[SingleSyncEngine] stop auto subscribe timerId=%d finished", subscribeTimerId_);
    RuntimeContext::GetInstance()->RemoveTimer(subscribeTimerId_);
    subscribeTimerId_ = 0;
}

int SingleVerSyncEngine::SubscribeTimeOut(TimerId id)
{
    if (!queryAutoSyncCallback_) {
        return E_OK;
    }
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    std::map<std::string, std::vector<QuerySyncObject>> allSyncQueries;
    GetAllUnFinishSubQueries(allSyncQueries);
    LOGI("[SingleVerSyncEngine] SubscribeTimeOut,size=%d", allSyncQueries.size());
    if (allSyncQueries.size() == 0) {
        LOGI("no need to trigger auto subscribe");
        return E_OK;
    }
    for (auto &item : allSyncQueries) {
        for (auto &query : item.second) {
            InternalSyncParma param;
            GetSubscribeSyncParam(item.first, query, param);
            queryAutoSyncCallback_(param);
        }
    }
    return E_OK;
}

void SingleVerSyncEngine::SetIsNeedResetAbilitySync(const std::string &deviceId, bool isNeedReset)
{
    ISyncTaskContext *context = GetSyncTaskContextAndInc(deviceId);
    if (context != nullptr) {
        context->SetIsNeedResetAbilitySync(isNeedReset);
        RefObject::DecObjRef(context);
    }
}
DEFINE_OBJECT_TAG_FACILITIES(SingleVerSyncEngine);
} // namespace DistributedDB
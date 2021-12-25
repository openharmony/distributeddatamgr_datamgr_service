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

#ifndef SINGLE_VER_SYNC_ENGINE_H
#define SINGLE_VER_SYNC_ENGINE_H

#include "sync_engine.h"

namespace DistributedDB {
class SingleVerSyncEngine final : public SyncEngine {
public:
    SingleVerSyncEngine() : needClearRemoteStaleData_(false) {};

    // If set true, remote stale data will be clear when remote db rebuilt.
    void EnableClearRemoteStaleData(bool enable);

    // used by SingleVerKVSyncer when db online
    int StartAutoSubscribeTimer() override;

    // used by SingleVerKVSyncer when remote/local db closed
    void StopAutoSubscribeTimer() override;

    int SubscribeTimeOut(TimerId id);

    void SetIsNeedResetAbilitySync(const std::string &deviceId, bool isNeedReset);

    DISABLE_COPY_ASSIGN_MOVE(SingleVerSyncEngine);
protected:
    ~SingleVerSyncEngine() override {};

    // Create a context
    ISyncTaskContext *CreateSyncTaskContext() override;

private:
    DECLARE_OBJECT_TAG(SingleVerSyncEngine);
#ifndef RUNNING_ON_TESTCASE
    static constexpr int SUBSCRIBE_TRIGGER_TIME_OUT = 30 * 60 * 1000; // 30min
#else
    static constexpr int SUBSCRIBE_TRIGGER_TIME_OUT = 5 * 60 * 1000; // 5min for test
#endif

    bool needClearRemoteStaleData_;

    // for subscribe timeout callback
    std::mutex timerLock_;
    TimerId subscribeTimerId_ = 0;
};
} // namespace DistributedDB

#endif // SINGLE_VER_SYNC_ENGINE_H
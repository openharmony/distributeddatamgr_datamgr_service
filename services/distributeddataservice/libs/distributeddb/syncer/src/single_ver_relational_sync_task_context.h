/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SINGLE_VER_RELATIONAL_SYNC_TASK_CONTEXT_H
#define SINGLE_VER_RELATIONAL_SYNC_TASK_CONTEXT_H

#ifdef RELATIONAL_STORE
#include "single_ver_sync_task_context.h"

namespace DistributedDB {
class SingleVerRelationalSyncTaskContext : public SingleVerSyncTaskContext {
public:

    explicit SingleVerRelationalSyncTaskContext();

    DISABLE_COPY_ASSIGN_MOVE(SingleVerRelationalSyncTaskContext);

    void Clear() override;
    std::string GetQuerySyncId() const override;
    std::string GetDeleteSyncId() const override;

    void SetRelationalSyncStrategy(RelationalSyncStrategy strategy);
    SyncStrategy GetSyncStrategy(QuerySyncObject &querySyncObject) const override;

    void SetIsNeedResetAbilitySync(bool isNeedReset) override;

    void SchemaChange() override;
protected:
    ~SingleVerRelationalSyncTaskContext() override;
    void CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam) override;

    std::string querySyncId_;
    std::string deleteSyncId_;

    // for relational syncStrategy
    mutable std::mutex syncStrategyMutex_;
    RelationalSyncStrategy relationalSyncStrategy_;
};
}
#endif // RELATIONAL_STORE
#endif // SINGLE_VER_RELATIONAL_SYNC_TASK_CONTEXT_H
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

#include "single_ver_kv_sync_task_context.h"

namespace DistributedDB {
SingleVerKvSyncTaskContext::SingleVerKvSyncTaskContext()
    : SingleVerSyncTaskContext(), syncStrategy_{}
{}

SingleVerKvSyncTaskContext::~SingleVerKvSyncTaskContext()
{
}

std::string SingleVerKvSyncTaskContext::GetQuerySyncId() const
{
    return query_.GetIdentify();
}

std::string SingleVerKvSyncTaskContext::GetDeleteSyncId() const
{
    return GetDeviceId();
}

void SingleVerKvSyncTaskContext::SetSyncStrategy(const SyncStrategy &strategy)
{
    syncStrategy_.permitSync = strategy.permitSync;
    syncStrategy_.convertOnSend = strategy.convertOnSend;
    syncStrategy_.convertOnReceive = strategy.convertOnReceive;
    syncStrategy_.checkOnReceive = strategy.checkOnReceive;
}

SyncStrategy SingleVerKvSyncTaskContext::GetSyncStrategy(QuerySyncObject &querySyncObject) const
{
    (void) querySyncObject;
    return syncStrategy_;
}
}
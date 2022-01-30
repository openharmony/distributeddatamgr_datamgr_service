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

#define LOG_TAG "RdbSyncerFactory"

#include "rdb_syncer_factory.h"
#include "log_print.h"
#include "rdb_types.h"

namespace OHOS::DistributedRdb {
RdbSyncerFactory& RdbSyncerFactory::GetInstance()
{
    static RdbSyncerFactory factory;
    return factory;
}

void RdbSyncerFactory::Register(int type, const Creator &creator)
{
    if (creator == nullptr) {
        ZLOGE("creator is empty");
        return;
    }
    ZLOGI("add creator for store type %{public}d", type);
    creators_.insert({ type, creator });
}

void RdbSyncerFactory::UnRegister(int type)
{
    creators_.erase(type);
}

RdbSyncerImpl* RdbSyncerFactory::CreateSyncer(const RdbSyncerParam& param, pid_t uid)
{
    auto it = creators_.find(param.type_);
    if (it == creators_.end()) {
        return nullptr;
    }
    return it->second(param, uid);
}
}


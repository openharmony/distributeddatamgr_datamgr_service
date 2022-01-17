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

#define LOG_TAG "RdbStoreFactory"

#include "rdb_store_factory.h"
#include "log_print.h"
#include "rdb_device_store.h"

namespace OHOS::DistributedKv {
RdbStoreFactory::Creator RdbStoreFactory::creators_[RDB_DISTRIBUTED_TYPE_MAX];

void RdbStoreFactory::Initialize()
{
    RdbDeviceStore::Initialize();
}

int RdbStoreFactory::RegisterCreator(int type, Creator &creator)
{
    if (type < 0 || type >= RDB_DISTRIBUTED_TYPE_MAX) {
        ZLOGE("type=%{public}d is invalid", type);
        return -1;
    }
    if (creator == nullptr) {
        ZLOGE("creator is empty");
        return -1;
    }
    ZLOGI("add creator for store type %{public}d", type);
    creators_[type] = creator;
    return 0;
}

RdbStore* RdbStoreFactory::CreateStore(const RdbStoreParam& param)
{
    int type = param.type_;
    if (type < 0 || type >= RDB_DISTRIBUTED_TYPE_MAX) {
        ZLOGE("type=%{public}d is invalid", type);
        return nullptr;
    }
    
    return (creators_[type])(param);
}
}


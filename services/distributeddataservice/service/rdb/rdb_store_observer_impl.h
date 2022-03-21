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

#ifndef DISTRIBUTED_RDB_STORE_OBSERVER_H
#define DISTRIBUTED_RDB_STORE_OBSERVER_H

#include <sys/types.h>
#include "store_observer.h"

namespace OHOS::DistributedRdb {
class RdbServiceImpl;
class RdbStoreObserverImpl : public DistributedDB::StoreObserver {
public:
    explicit RdbStoreObserverImpl(RdbServiceImpl* owner, pid_t pid = 0);

    ~RdbStoreObserverImpl() override;

    void OnChange(const DistributedDB::StoreChangedData &data) override;

private:
    pid_t pid_ {};
    RdbServiceImpl* owner_ {};
};
} // namespace OHOS::DistributedRdb
#endif

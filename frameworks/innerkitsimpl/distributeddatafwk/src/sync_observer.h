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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H

#include <vector>
#include <memory>
#include <mutex>
#include "kvstore_sync_callback.h"

namespace OHOS::DistributedKv {
class SyncObserver : public KvStoreSyncCallback {
public:
    explicit SyncObserver(const std::vector<std::shared_ptr<KvStoreSyncCallback>> &callbacks);

    SyncObserver();

    virtual ~SyncObserver() = default;

    bool Add(const std::shared_ptr<KvStoreSyncCallback> callback);

    bool Clean();

    void SyncCompleted(const std::map<std::string, DistributedKv::Status> &results) override;

private:
    std::recursive_mutex mutex_;
    std::vector<std::shared_ptr<KvStoreSyncCallback>> callbacks_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_DATAMGR_SYNC_OBSERVER_H

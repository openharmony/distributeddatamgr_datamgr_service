/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_WATCHER_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_WATCHER_H
#include <mutex>
#include <shared_mutex>

#include "ikvstore_observer.h"
#include "kv_store_changed_data.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

namespace OHOS::DistributedKv {
class KVDBWatcher : public DistributedData::GeneralWatcher {
public:
    KVDBWatcher();
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override;
    int32_t OnChange(const Origin &origin, const GeneralWatcher::Fields &fields, ChangeData &&datas) override;
    std::set<sptr<KvStoreObserverProxy>> GetObservers() const;
    void SetObservers(std::set<sptr<KvStoreObserverProxy>> observers);
    void ClearObservers();

private:
    std::vector<Entry> ConvertToEntries(const std::vector<DistributedData::Values> &values);
    std::vector<std::string> ConvertToKeys(const std::vector<PRIValue> &values);
    mutable std::shared_mutex mutex_;
    std::set<sptr<KvStoreObserverProxy>> observers_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_WATCHER_H

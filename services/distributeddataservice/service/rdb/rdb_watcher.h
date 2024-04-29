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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_WATCHER_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_WATCHER_H
#include <mutex>
#include <shared_mutex>
#include "rdb_notifier_proxy.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

namespace OHOS::DistributedRdb {
class RdbWatcher : public DistributedData::GeneralWatcher {
public:
    RdbWatcher();
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override;
    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override;
    sptr<RdbNotifierProxy> GetNotifier() const;
    void SetNotifier(sptr<RdbNotifierProxy> notifier);

private:
    mutable std::shared_mutex mutex_;
    sptr<RdbNotifierProxy> notifier_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_WATCHER_H

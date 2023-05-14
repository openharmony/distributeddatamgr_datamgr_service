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
#include "rdb_service_impl.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

namespace OHOS::DistributedRdb {
class RdbServiceImpl;
class RdbWatcher : public DistributedData::GeneralWatcher {
public:
    explicit RdbWatcher(RdbServiceImpl *rdbService, uint32_t tokenId, const std::string &storeName);
    int32_t OnChange(Origin origin, const std::string &id) override;
    int32_t OnChange(Origin origin, const std::string &id,
        const std::vector<DistributedData::VBucket> &values) override;

private:
    RdbServiceImpl* rdbService_ {};
    uint32_t tokenId_ = 0;
    std::string storeName_ {};
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_WATCHER_H

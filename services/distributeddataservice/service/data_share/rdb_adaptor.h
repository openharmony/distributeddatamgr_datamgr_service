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

#ifndef DATASHARESERVICE_RDB_ADAPTOR_H
#define DATASHARESERVICE_RDB_ADAPTOR_H

#include <mutex>
#include <string>

#include "concurrent_map.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"
#include "metadata/store_meta_data.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "timer.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using namespace OHOS::NativeRdb;
class RdbDelegate {
public:
    explicit RdbDelegate(const StoreMetaData &data);
    virtual ~RdbDelegate();
    int64_t Insert(const std::string &tableName, const DataShareValuesBucket &valuesBucket);
    int64_t Update(
        const std::string &tableName, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket);
    int64_t Delete(const std::string &tableName, const DataSharePredicates &predicate);
    std::shared_ptr<DataShareResultSet> Query(
        const std::string &tableName, const DataSharePredicates &predicates, const std::vector<std::string> &columns);

private:
    std::shared_ptr<RdbStore> store_;
};
class RdbAdaptor {
public:
    static int32_t Insert(const UriInfo &uriInfo, const DataShareValuesBucket &valuesBucket, const int32_t userId);
    static int32_t Update(const UriInfo &uriInfo, const DataSharePredicates &predicate,
        const DataShareValuesBucket &valuesBucket, const int32_t userId);
    static int32_t Delete(const UriInfo &uriInfo, const DataSharePredicates &predicate, const int32_t userId);
    static std::shared_ptr<DataShareResultSet> Query(const UriInfo &uriInfo, const DataSharePredicates &predicates,
        const std::vector<std::string> &columns, const int32_t userId);

private:
    static constexpr int OPEN_TIME = 30000; // 30s
    static std::shared_ptr<RdbDelegate> GetDelegate(const UriInfo &uriInfo, const int32_t userId);
    static void AutoClose();
    static ConcurrentMap<UriInfo, std::shared_ptr<RdbDelegate>> delegates_;
    static std::unique_ptr<Utils::Timer> timer_;
    static uint32_t timerId_;
};
class DefaultOpenCallback : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &rdbStore) override
    {
        return E_OK;
    }
    int OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return E_OK;
    }
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_RDB_ADAPTOR_H

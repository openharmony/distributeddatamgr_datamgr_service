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

#ifndef DATASHARESERVICE_RDB_DELEGATE_H
#define DATASHARESERVICE_RDB_DELEGATE_H

#include <mutex>
#include <string>

#include "concurrent_map.h"
#include "db_delegate.h"
#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_store.h"
#include "uri_utils.h"
#include "rdb_utils.h"

namespace OHOS::DataShare {
using namespace OHOS::NativeRdb;
class RdbDelegate final : public DBDelegate {
public:
    explicit RdbDelegate(const std::string &dir, int version, bool registerFunction,
        bool isEncrypt, const std::string &secretMetaKey);
    int64_t Insert(const std::string &tableName, const DataShareValuesBucket &valuesBucket) override;
    int64_t Update(const std::string &tableName, const DataSharePredicates &predicate,
        const DataShareValuesBucket &valuesBucket) override;
    int64_t Delete(const std::string &tableName, const DataSharePredicates &predicate) override;
    std::pair<int, std::shared_ptr<DataShareResultSet>> Query(const std::string &tableName,
        const DataSharePredicates &predicates, const std::vector<std::string> &columns,
        const int32_t callingPid) override;
    std::string Query(const std::string &sql, const std::vector<std::string> &selectionArgs) override;
    std::shared_ptr<NativeRdb::ResultSet> QuerySql(const std::string &sql) override;

private:
    static std::atomic<int32_t> resultSetCount;
    std::shared_ptr<RdbStore> store_;
    int errCode_ = E_OK;
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
#endif // DATASHARESERVICE_RDB_DELEGATE_H

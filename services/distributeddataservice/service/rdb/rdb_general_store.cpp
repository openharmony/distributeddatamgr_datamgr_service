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
#define LOG_TAG "RdbGeneralStore"
#include "rdb_general_store.h"

#include "crypto_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "rdb_helper.h"
#include "relational_store_manager.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
using namespace OHOS::NativeRdb;
class RdbOpenCallbackImpl : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &rdbStore) override
    {
        return NativeRdb::E_OK;
    }
    int OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion) override
    {
        return NativeRdb::E_OK;
    }
};
RdbGeneralStore::RdbGeneralStore(const StoreMetaData &meta) : manager_(meta.appId, meta.user, meta.instanceId)
{
}

int32_t RdbGeneralStore::Close()
{
    return 0;
}

int32_t RdbGeneralStore::Execute(const std::string &table, const std::string &sql)
{
    return GeneralError::E_OK;
}

int32_t RdbGeneralStore::BatchInsert(const std::string &table, VBuckets &&values)
{
    return 0;
}

int32_t RdbGeneralStore::BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values)
{
    return 0;
}

int32_t RdbGeneralStore::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return 0;
}

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, const std::string &sql, Values &&args)
{
    return std::shared_ptr<Cursor>();
}

std::shared_ptr<Cursor> RdbGeneralStore::Query(const std::string &table, GenQuery &query)
{
    return std::shared_ptr<Cursor>();
}

int32_t RdbGeneralStore::Watch(int32_t origin, Watcher &watcher)
{
    return 0;
}

int32_t RdbGeneralStore::Unwatch(int32_t origin, Watcher &watcher)
{
    return 0;
}

int32_t RdbGeneralStore::Sync(const Devices &devices, int32_t mode, GenQuery &query, Async async, int32_t wait)
{
    return 0;
}

int32_t RdbGeneralStore::Bind(std::shared_ptr<CloudDB> cloudDb)
{
    cloudDb_ = std::move(cloudDb);
    return 0;
}

int32_t RdbGeneralStore::SetSchema(const SchemaMeta &schemaMeta)
{
    //delegate_->SetSchema(schemaMeta);
    return GeneralError::E_OK;
}
} // namespace OHOS::DistributedRdb

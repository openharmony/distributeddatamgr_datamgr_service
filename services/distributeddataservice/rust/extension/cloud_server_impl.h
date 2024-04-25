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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_SERVER_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_SERVER_IMPL_H

#include "basic_rust_types.h"
#include "cloud_db_impl.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_server.h"
#include "cloud/subscription.h"

namespace OHOS::CloudData {
using DBMeta = DistributedData::SchemaMeta::Database;
using DBAssetLoader = DistributedData::AssetLoader;
using DBCloudDB = DistributedData::CloudDB;
using DBCloudInfo = DistributedData::CloudInfo;
using DBSchemaMeta = DistributedData::SchemaMeta;
using DBTable = DistributedData::Table;
using DBField = DistributedData::Field;
using DBSub = DistributedData::Subscription;
using DBRelation = DBSub::Relation;
using DBErr = DistributedData::GeneralError;
class CloudServerImpl : public DistributedData::CloudServer {
public:
    DBCloudInfo GetServerInfo(int32_t userId) override;
    DBSchemaMeta GetAppSchema(int32_t userId, const std::string &bundleName) override;
    int32_t Subscribe(int32_t userId, const std::map<std::string, std::vector<DBMeta>> &dbs) override;
    int32_t Unsubscribe(int32_t userId, const std::map<std::string, std::vector<DBMeta>> &dbs) override;
    std::shared_ptr<DBAssetLoader> ConnectAssetLoader(uint32_t tokenId, const DBMeta &dbMeta) override;
    std::shared_ptr<DBAssetLoader> ConnectAssetLoader(
        const std::string &bundleName, int user, const DBMeta &dbMeta) override;
    std::shared_ptr<DBCloudDB> ConnectCloudDB(uint32_t tokenId, const DBMeta &dbMeta) override;
    std::shared_ptr<DBCloudDB> ConnectCloudDB(const std::string &bundleName, int user, const DBMeta &dbMeta) override;

private:
    static constexpr uint64_t INTERVAL = 6 * 24;
    void GetAppInfo(std::shared_ptr<OhCloudExtHashMap> briefInfo, DBCloudInfo &cloudInfo);
    void GetDatabases(std::shared_ptr<OhCloudExtVector> databases, DBSchemaMeta &dbSchema);
    void GetTables(std::shared_ptr<OhCloudExtHashMap> tables, DBMeta &dbMeta);
    void GetTableInfo(std::shared_ptr<OhCloudExtTable> pTable, DBTable &dbTable);
    void GetFields(std::shared_ptr<OhCloudExtVector> fields, DBTable &dbTable);
    int32_t DoSubscribe(int32_t userId, std::shared_ptr<OhCloudExtCloudSync> server,
        std::shared_ptr<OhCloudExtHashMap> databases);
    int32_t SaveSubscription(int32_t userId, std::shared_ptr<OhCloudExtHashMap> relations,
        std::shared_ptr<OhCloudExtCloudSync> server);
    int32_t SaveRelation(std::shared_ptr<OhCloudExtVector> keys,
        std::shared_ptr<OhCloudExtVector> values, DBSub &sub);
    int32_t GetRelation(std::shared_ptr<OhCloudExtHashMap> relations, DBRelation &dbRelation);
    int32_t DoUnsubscribe(std::shared_ptr<OhCloudExtCloudSync> server,
        std::shared_ptr<OhCloudExtHashMap> relations, const std::vector<std::string> &bundles, DBSub &sub);
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_SERVER_IMPL_H

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

#define LOG_TAG "ExtensionCloudServerImpl"
#include "cloud_server_impl.h"
#include "accesstoken_kit.h"
#include "asset_loader_impl.h"
#include "cloud_extension.h"
#include "cloud_ext_types.h"
#include "cloud/subscription.h"
#include "error.h"
#include "error/general_error.h"
#include "extension_util.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"

namespace OHOS::CloudData {
__attribute__((used)) static bool g_isInit =
    DistributedData::CloudServer::RegisterCloudInstance(new (std::nothrow) CloudServerImpl());
using namespace Security::AccessToken;
using DBMetaMgr = DistributedData::MetaDataManager;
using Anonymous = DistributedData::Anonymous;
DBCloudInfo CloudServerImpl::GetServerInfo(int32_t userId)
{
    DBCloudInfo result;
    OhCloudExtCloudSync *server = OhCloudExtCloudSyncNew(userId);
    if (server == nullptr) {
        return result;
    }
    auto pServer = std::shared_ptr<OhCloudExtCloudSync>(server, [](auto *server) {
        OhCloudExtCloudSyncFree(server);
    });
    OhCloudExtCloudInfo *info = nullptr;
    auto status = OhCloudExtCloudSyncGetServiceInfo(pServer.get(), &info);
    if (status != ERRNO_SUCCESS || info == nullptr) {
        return result;
    }
    auto pInfo = std::shared_ptr<OhCloudExtCloudInfo>(info, [](auto *info) { OhCloudExtCloudInfoFree(info); });
    status = OhCloudExtCloudInfoGetUser(pInfo.get(), &result.user);
    if (status != ERRNO_SUCCESS || result.user != userId) {
        ZLOGE("[IN]user: %{public}d, [OUT]user: %{public}d", userId, result.user);
        return result;
    }
    unsigned char *id = nullptr;
    size_t idLen = 0;
    status = OhCloudExtCloudInfoGetId(pInfo.get(), &id, reinterpret_cast<unsigned int *>(&idLen));
    if (status != ERRNO_SUCCESS || id == nullptr) {
        return result;
    }
    result.id = std::string(reinterpret_cast<char *>(id), idLen);
    unsigned long long totalSpace = 0;
    OhCloudExtCloudInfoGetTotalSpace(pInfo.get(), &totalSpace);
    result.totalSpace = totalSpace;
    unsigned long long remainSpace = 0;
    OhCloudExtCloudInfoGetRemainSpace(pInfo.get(), &remainSpace);
    result.remainSpace = remainSpace;
    OhCloudExtCloudInfoEnabled(pInfo.get(), &result.enableCloud);
    OhCloudExtHashMap *briefInfo = nullptr;
    status = OhCloudExtCloudInfoGetAppInfo(pInfo.get(), &briefInfo);
    if (status != ERRNO_SUCCESS || briefInfo == nullptr) {
        return result;
    }
    auto pBriefInfo = std::shared_ptr<OhCloudExtHashMap>(briefInfo, [](auto *briefInfo) {
        OhCloudExtHashMapFree(briefInfo);
    });
    GetAppInfo(pBriefInfo, result);
    return result;
}

void CloudServerImpl::GetAppInfo(std::shared_ptr<OhCloudExtHashMap> briefInfo, DBCloudInfo &cloudInfo)
{
    OhCloudExtVector *keys = nullptr;
    OhCloudExtVector *values = nullptr;
    auto status = OhCloudExtHashMapIterGetKeyValuePair(briefInfo.get(), &keys, &values);
    if (status != ERRNO_SUCCESS || keys == nullptr || values == nullptr) {
        return;
    }
    auto pKeys = std::shared_ptr<OhCloudExtVector>(keys, [](auto *keys) { OhCloudExtVectorFree(keys); });
    auto pValues = std::shared_ptr<OhCloudExtVector>(values, [](auto *values) { OhCloudExtVectorFree(values); });
    size_t keysLen = 0;
    OhCloudExtVectorGetLength(pKeys.get(), reinterpret_cast<unsigned int *>(&keysLen));
    size_t valuesLen = 0;
    OhCloudExtVectorGetLength(pValues.get(), reinterpret_cast<unsigned int *>(&valuesLen));
    if (keysLen == 0 || keysLen != valuesLen) {
        return;
    }
    for (size_t i = 0; i < keysLen; i++) {
        void *key = nullptr;
        size_t keyLen = 0;
        status = OhCloudExtVectorGet(pKeys.get(), i, &key, reinterpret_cast<unsigned int *>(&keyLen));
        if (status != ERRNO_SUCCESS || key == nullptr) {
            return;
        }
        void *value = nullptr;
        size_t valueLen = 0;
        status = OhCloudExtVectorGet(pValues.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS || value == nullptr) {
            return;
        }
        std::string bundle(reinterpret_cast<char *>(key), keyLen);
        OhCloudExtAppInfo *appInfo = reinterpret_cast<OhCloudExtAppInfo *>(value);
        cloudInfo.apps[bundle] = ExtensionUtil::ConvertAppInfo(appInfo);
        OhCloudExtAppInfoFree(appInfo);
    }
}

DBSchemaMeta CloudServerImpl::GetAppSchema(int32_t userId, const std::string &bundleName)
{
    DBSchemaMeta dbSchema;
    dbSchema.bundleName = bundleName;
    OhCloudExtCloudSync *server = OhCloudExtCloudSyncNew(userId);
    if (server == nullptr) {
        return dbSchema;
    }
    auto pServer = std::shared_ptr<OhCloudExtCloudSync>(server, [](auto *server) {
        OhCloudExtCloudSyncFree(server);
    });
    OhCloudExtSchemaMeta *schema = nullptr;
    auto status = OhCloudExtCloudSyncGetAppSchema(pServer.get(),
        reinterpret_cast<const unsigned char *>(bundleName.c_str()), bundleName.size(), &schema);
    if (status != ERRNO_SUCCESS || schema == nullptr) {
        return dbSchema;
    }
    auto pSchema = std::shared_ptr<OhCloudExtSchemaMeta>(schema, [](auto *schema) {
        OhCloudExtSchemaMetaFree(schema);
    });
    OhCloudExtSchemaMetaGetVersion(pSchema.get(), &dbSchema.version);
    OhCloudExtVector *databases = nullptr;
    status = OhCloudExtSchemaMetaGetDatabases(pSchema.get(), &databases);
    if (status != ERRNO_SUCCESS || databases == nullptr) {
        return dbSchema;
    }
    auto pDatabases = std::shared_ptr<OhCloudExtVector>(databases, [](auto *databases) {
        OhCloudExtVectorFree(databases);
    });
    GetDatabases(pDatabases, dbSchema);
    return dbSchema;
}

void CloudServerImpl::GetDatabases(std::shared_ptr<OhCloudExtVector> databases, DBSchemaMeta &dbSchema)
{
    size_t dbsLen = 0;
    auto status = OhCloudExtVectorGetLength(databases.get(), reinterpret_cast<unsigned int *>(&dbsLen));
    if (status != ERRNO_SUCCESS || dbsLen == 0) {
        return;
    }
    dbSchema.databases.reserve(dbsLen);
    for (size_t i = 0; i < dbsLen; i++) {
        void *database = nullptr;
        size_t dbLen = 0;
        status = OhCloudExtVectorGet(databases.get(), i, &database, reinterpret_cast<unsigned int *>(&dbLen));
        if (status != ERRNO_SUCCESS || database == nullptr) {
            return;
        }
        OhCloudExtDatabase *db = reinterpret_cast<OhCloudExtDatabase *>(database);
        auto pDb = std::shared_ptr<OhCloudExtDatabase>(db, [](auto *db) { OhCloudExtDatabaseFree(db); });
        DBMeta dbMeta;
        unsigned char *name = nullptr;
        size_t nameLen = 0;
        OhCloudExtDatabaseGetName(pDb.get(), &name, reinterpret_cast<unsigned int *>(&nameLen));
        if (name == nullptr) {
            return;
        }
        dbMeta.name = std::string(reinterpret_cast<char *>(name), nameLen);
        unsigned char *alias = nullptr;
        size_t aliasLen = 0;
        OhCloudExtDatabaseGetAlias(pDb.get(), &alias, reinterpret_cast<unsigned int *>(&aliasLen));
        if (alias == nullptr) {
            return;
        }
        dbMeta.alias = std::string(reinterpret_cast<char *>(alias), aliasLen);
        OhCloudExtHashMap *tables = nullptr;
        OhCloudExtDatabaseGetTable(pDb.get(), &tables);
        if (tables == nullptr) {
            return;
        }
        auto pTables = std::shared_ptr<OhCloudExtHashMap>(tables, [](auto *tables) {
            OhCloudExtHashMapFree(tables);
        });
        GetTables(pTables, dbMeta);
        dbSchema.databases.emplace_back(std::move(dbMeta));
    }
}

void CloudServerImpl::GetTables(std::shared_ptr<OhCloudExtHashMap> tables, DBMeta &dbMeta)
{
    OhCloudExtVector *keys = nullptr;
    OhCloudExtVector *values = nullptr;
    auto status = OhCloudExtHashMapIterGetKeyValuePair(tables.get(), &keys, &values);
    if (status != ERRNO_SUCCESS || keys == nullptr || values == nullptr) {
        return;
    }
    auto pKeys = std::shared_ptr<OhCloudExtVector>(keys, [](auto *keys) { OhCloudExtVectorFree(keys); });
    auto pValues = std::shared_ptr<OhCloudExtVector>(values, [](auto *values) { OhCloudExtVectorFree(values); });
    size_t keysLen = 0;
    OhCloudExtVectorGetLength(pKeys.get(), reinterpret_cast<unsigned int *>(&keysLen));
    size_t valuesLen = 0;
    OhCloudExtVectorGetLength(pValues.get(), reinterpret_cast<unsigned int *>(&valuesLen));
    if (keysLen == 0 || keysLen != valuesLen) {
        return;
    }
    for (size_t i = 0; i < valuesLen; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        status = OhCloudExtVectorGet(pValues.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS || value == nullptr) {
            return;
        }
        DBTable dbTable;
        OhCloudExtTable *table = reinterpret_cast<OhCloudExtTable *>(value);
        auto pTable = std::shared_ptr<OhCloudExtTable>(table, [](auto *table) { OhCloudExtTableFree(table); });
        GetTableInfo(pTable, dbTable);
        OhCloudExtVector *fields = nullptr;
        status = OhCloudExtTableGetFields(pTable.get(), &fields);
        if (status != ERRNO_SUCCESS || fields == nullptr) {
            return;
        }
        auto pFields = std::shared_ptr<OhCloudExtVector>(fields, [](auto *fields) {
            OhCloudExtVectorFree(fields);
        });
        GetFields(pFields, dbTable);
        dbMeta.tables.emplace_back(std::move(dbTable));
    }
}

void CloudServerImpl::GetTableInfo(std::shared_ptr<OhCloudExtTable> pTable, DBTable &dbTable)
{
    unsigned char *tbName = nullptr;
    size_t tbNameLen = 0;
    OhCloudExtTableGetName(pTable.get(), &tbName, reinterpret_cast<unsigned int *>(&tbNameLen));
    if (tbName == nullptr) {
        return;
    }
    dbTable.name = std::string(reinterpret_cast<char *>(tbName), tbNameLen);
    unsigned char *tbAlias = nullptr;
    size_t tbAliasLen = 0;
    OhCloudExtTableGetAlias(pTable.get(), &tbAlias, reinterpret_cast<unsigned int *>(&tbAliasLen));
    if (tbAlias == nullptr) {
        return;
    }
    dbTable.alias = std::string(reinterpret_cast<char *>(tbAlias), tbAliasLen);
}

void CloudServerImpl::GetFields(std::shared_ptr<OhCloudExtVector> fields, DBTable &dbTable)
{
    size_t fieldsLen = 0;
    auto status = OhCloudExtVectorGetLength(fields.get(), reinterpret_cast<unsigned int *>(&fieldsLen));
    if (status != ERRNO_SUCCESS || fieldsLen == 0) {
        return;
    }
    dbTable.fields.reserve(fieldsLen);
    for (size_t i = 0; i < fieldsLen; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        status = OhCloudExtVectorGet(fields.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (status != ERRNO_SUCCESS || value == nullptr) {
            return;
        }
        OhCloudExtField *field = reinterpret_cast<OhCloudExtField *>(value);
        auto pField = std::shared_ptr<OhCloudExtField>(field, [](auto *field) { OhCloudExtFieldFree(field); });
        DBField dbField;
        unsigned char *colName = nullptr;
        size_t colLen = 0;
        OhCloudExtFieldGetColName(pField.get(), &colName, reinterpret_cast<unsigned int *>(&colLen));
        if (colName == nullptr) {
            return;
        }
        dbField.colName = std::string(reinterpret_cast<char *>(colName), colLen);
        unsigned char *fdAlias = nullptr;
        size_t fdAliasLen = 0;
        OhCloudExtFieldGetAlias(pField.get(), &fdAlias, reinterpret_cast<unsigned int *>(&fdAliasLen));
        if (fdAlias == nullptr) {
            return;
        }
        dbField.alias = std::string(reinterpret_cast<char *>(fdAlias), fdAliasLen);
        uint32_t fdtype;
        OhCloudExtFieldGetTyp(pField.get(), &fdtype);
        dbField.type = static_cast<int32_t>(fdtype);
        bool primary = false;
        OhCloudExtFieldGetPrimary(pField.get(), &primary);
        dbField.primary = primary;
        bool nullable = true;
        OhCloudExtFieldGetNullable(pField.get(), &nullable);
        dbField.nullable = nullable;
        dbTable.fields.emplace_back(std::move(dbField));
    }
}

int32_t CloudServerImpl::Subscribe(int32_t userId, const std::map<std::string, std::vector<DBMeta>> &dbs)
{
    OhCloudExtCloudSync *server = OhCloudExtCloudSyncNew(userId);
    if (server == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pServer = std::shared_ptr<OhCloudExtCloudSync>(server, [](auto *server) {
        OhCloudExtCloudSyncFree(server);
    });
    OhCloudExtHashMap *databases = OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_VEC_DATABASE);
    if (databases == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pDatabases = std::shared_ptr<OhCloudExtHashMap>(databases, [](auto *databases) {
        OhCloudExtHashMapFree(databases);
    });
    for (auto &[bundle, db] : dbs) {
        OhCloudExtVector *database = OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_DATABASE);
        if (database == nullptr) {
            return DBErr::E_ERROR;
        }
        size_t databaseLen = 0;
        for (auto &item : db) {
            auto data = ExtensionUtil::Convert(item);
            if (data.first == nullptr) {
                return DBErr::E_ERROR;
            }
            auto status = OhCloudExtVectorPush(database, data.first, data.second);
            if (status != ERRNO_SUCCESS) {
                return DBErr::E_ERROR;
            }
            databaseLen += 1;
        }
        auto status = OhCloudExtHashMapInsert(pDatabases.get(),
            const_cast<void *>(reinterpret_cast<const void *>(bundle.c_str())), bundle.size(), database, databaseLen);
        if (status != ERRNO_SUCCESS) {
            return DBErr::E_ERROR;
        }
    }
    return DoSubscribe(userId, pServer, pDatabases);
}

int32_t CloudServerImpl::DoSubscribe(int32_t userId, std::shared_ptr<OhCloudExtCloudSync> server,
    std::shared_ptr<OhCloudExtHashMap> databases)
{
    auto expire = std::chrono::duration_cast<std::chrono::milliseconds>
        ((std::chrono::system_clock::now() + std::chrono::hours(INTERVAL)).time_since_epoch()).count();
    OhCloudExtHashMap *relations = nullptr;
    OhCloudExtVector *errs = nullptr;
    auto status = OhCloudExtCloudSyncSubscribe(server.get(), databases.get(), expire, &relations, &errs);
    if (status != ERRNO_SUCCESS || relations == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pRelations = std::shared_ptr<OhCloudExtHashMap>(relations, [](auto *relations) {
        OhCloudExtHashMapFree(relations);
    });
    if (errs != nullptr) {
        auto pErrs = std::shared_ptr<OhCloudExtVector>(errs, [](auto *errs) { OhCloudExtVectorFree(errs); });
        size_t errsLen = 0;
        status = OhCloudExtVectorGetLength(pErrs.get(), reinterpret_cast<unsigned int *>(&errsLen));
        if (status != ERRNO_SUCCESS || errsLen == 0) {
            return DBErr::E_ERROR;
        }
        for (size_t i = 0; i < errsLen; i++) {
            void *value = nullptr;
            size_t valueLen = 0;
            status = OhCloudExtVectorGet(pErrs.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
            if (status != ERRNO_SUCCESS || value == nullptr) {
                return DBErr::E_ERROR;
            }
            auto err = *reinterpret_cast<int *>(value);
            if (err != ERRNO_SUCCESS) {
                ZLOGE("sub fail, err:%{oublic}d", err);
                return DBErr::E_ERROR;
            }
        }
    }
    return SaveSubscription(userId, pRelations, server);
}

int32_t CloudServerImpl::SaveSubscription(int32_t userId, std::shared_ptr<OhCloudExtHashMap> relations,
    std::shared_ptr<OhCloudExtCloudSync> server)
{
    OhCloudExtCloudInfo *info = nullptr;
    auto status = OhCloudExtCloudSyncGetServiceInfo(server.get(), &info);
    if (status != ERRNO_SUCCESS || info == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pInfo = std::shared_ptr<OhCloudExtCloudInfo>(info, [](auto *info) { OhCloudExtCloudInfoFree(info); });
    unsigned char *id = nullptr;
    size_t idLen = 0;
    status = OhCloudExtCloudInfoGetId(pInfo.get(), &id, reinterpret_cast<unsigned int *>(&idLen));
    if (status != ERRNO_SUCCESS || id == nullptr) {
        return DBErr::E_ERROR;
    }
    std::string accountId(reinterpret_cast<char *>(id), idLen);
    DBSub sub;
    sub.userId = userId;
    DBMetaMgr::GetInstance().LoadMeta(sub.GetKey(), sub, true);
    if (!sub.id.empty() && sub.id != accountId) {
        ZLOGE("diff id, [meta]id:%{public}s, [server]id:%{public}s", Anonymous::Change(sub.id).c_str(),
              Anonymous::Change(accountId).c_str());
        return DBErr::E_OK;
    }
    sub.id = accountId;
    OhCloudExtVector *keys = nullptr;
    OhCloudExtVector *values = nullptr;
    status = OhCloudExtHashMapIterGetKeyValuePair(relations.get(), &keys, &values);
    if (status != ERRNO_SUCCESS || keys == nullptr || values == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pKeys = std::shared_ptr<OhCloudExtVector>(keys, [](auto *keys) { OhCloudExtVectorFree(keys); });
    auto pValues = std::shared_ptr<OhCloudExtVector>(values, [](auto *values) { OhCloudExtVectorFree(values); });
    if (SaveRelation(pKeys, pValues, sub) != DBErr::E_OK) {
        return DBErr::E_ERROR;
    }
    DBMetaMgr::GetInstance().SaveMeta(sub.GetKey(), sub, true);
    return DBErr::E_OK;
}

int32_t CloudServerImpl::SaveRelation(std::shared_ptr<OhCloudExtVector> keys,
    std::shared_ptr<OhCloudExtVector> values, DBSub &sub)
{
    size_t valuesLen = 0;
    auto status = OhCloudExtVectorGetLength(values.get(), reinterpret_cast<unsigned int *>(&valuesLen));
    if (status != ERRNO_SUCCESS || valuesLen == 0) {
        return DBErr::E_ERROR;
    }
    for (size_t i = 0; i < valuesLen; i++) {
        void *value = nullptr;
        size_t valueLen = 0;
        OhCloudExtVectorGet(values.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
        if (value == nullptr) {
            return DBErr::E_ERROR;
        }
        OhCloudExtRelationSet *relationSet = reinterpret_cast<OhCloudExtRelationSet *>(value);
        auto pRelationSet = std::shared_ptr<OhCloudExtRelationSet>(relationSet, [](auto *relationSet) {
            OhCloudExtRelationSetFree(relationSet);
        });
        unsigned char *bundle = nullptr;
        size_t bundleLen = 0;
        OhCloudExtRelationSetGetBundleName(pRelationSet.get(), &bundle, reinterpret_cast<unsigned int *>(&bundleLen));
        if (bundle == nullptr) {
            return DBErr::E_ERROR;
        }
        unsigned long long expire = 0;
        status = OhCloudExtRelationSetGetExpireTime(pRelationSet.get(), &expire);
        if (status != ERRNO_SUCCESS || expire == 0) {
            return DBErr::E_ERROR;
        }
        std::string bundleName(reinterpret_cast<char *>(bundle), bundleLen);
        sub.expiresTime[bundleName] = static_cast<uint64_t>(expire);
        OhCloudExtHashMap *relations = nullptr;
        status = OhCloudExtRelationSetGetRelations(pRelationSet.get(), &relations);
        if (status != ERRNO_SUCCESS || relations == nullptr) {
            return DBErr::E_ERROR;
        }
        auto pRelations = std::shared_ptr<OhCloudExtHashMap>(relations, [](auto *relations) {
            OhCloudExtHashMapFree(relations);
        });
        DBRelation dbRelation;
        dbRelation.id = sub.id;
        dbRelation.bundleName = bundleName;
        if (GetRelation(pRelations, dbRelation) != DBErr::E_OK) {
            return DBErr::E_ERROR;
        }
        DBMetaMgr::GetInstance().SaveMeta(sub.GetRelationKey(bundleName), dbRelation, true);
    }
    return DBErr::E_OK;
}

int32_t CloudServerImpl::GetRelation(std::shared_ptr<OhCloudExtHashMap> relations, DBRelation &dbRelation)
{
    OhCloudExtVector *keys = nullptr;
    OhCloudExtVector *values = nullptr;
    auto status = OhCloudExtHashMapIterGetKeyValuePair(relations.get(), &keys, &values);
    if (status != ERRNO_SUCCESS || keys == nullptr || values == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pKeys = std::shared_ptr<OhCloudExtVector>(keys, [](auto *keys) { OhCloudExtVectorFree(keys); });
    auto pValues = std::shared_ptr<OhCloudExtVector>(values, [](auto *values) { OhCloudExtVectorFree(values); });
    size_t keysLen = 0;
    OhCloudExtVectorGetLength(pKeys.get(), reinterpret_cast<unsigned int *>(&keysLen));
    size_t valuesLen = 0;
    OhCloudExtVectorGetLength(pValues.get(), reinterpret_cast<unsigned int *>(&valuesLen));
    if (keysLen == 0 || keysLen != valuesLen) {
        return DBErr::E_ERROR;
    }
    for (size_t i = 0; i < keysLen; i++) {
        void *dbName = nullptr;
        size_t dbNameLen = 0;
        status = OhCloudExtVectorGet(pKeys.get(), i, &dbName, reinterpret_cast<unsigned int *>(&dbNameLen));
        if (status != ERRNO_SUCCESS || dbName == nullptr) {
            return DBErr::E_ERROR;
        }
        std::string databaseName(reinterpret_cast<char *>(dbName), dbNameLen);
        void *subId = nullptr;
        size_t subIdLen = 0;
        status = OhCloudExtVectorGet(pValues.get(), i, &subId, reinterpret_cast<unsigned int *>(&subIdLen));
        if (status != ERRNO_SUCCESS || subId == nullptr) {
            return DBErr::E_ERROR;
        }
        uint64_t subscribeId = *reinterpret_cast<uint64_t *>(subId);
        dbRelation.relations[std::move(databaseName)] = std::to_string(subscribeId);
    }
    return DBErr::E_OK;
}

int32_t CloudServerImpl::Unsubscribe(int32_t userId, const std::map<std::string, std::vector<DBMeta>> &dbs)
{
    DBSub sub;
    sub.userId = userId;
    DBMetaMgr::GetInstance().LoadMeta(sub.GetKey(), sub, true);
    if (sub.id.empty()) {
        return DBErr::E_OK;
    }
    OhCloudExtCloudSync *server = OhCloudExtCloudSyncNew(userId);
    if (server == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pServer = std::shared_ptr<OhCloudExtCloudSync>(server, [](auto *server) { OhCloudExtCloudSyncFree(server); });
    std::vector<std::string> bundles;
    OhCloudExtHashMap *subs = OhCloudExtHashMapNew(OhCloudExtRustType::VALUETYPE_VEC_STRING);
    if (subs == nullptr) {
        return DBErr::E_ERROR;
    }
    auto pSubs = std::shared_ptr<OhCloudExtHashMap>(subs, [](auto *subs) { OhCloudExtHashMapFree(subs); });
    for (auto &[bundle, databases] : dbs) {
        DBRelation dbRelation;
        DBMetaMgr::GetInstance().LoadMeta(DBSub::GetRelationKey(userId, bundle), dbRelation, true);
        OhCloudExtVector *relation = OhCloudExtVectorNew(OhCloudExtRustType::VALUETYPE_U32);
        if (relation == nullptr) {
            return DBErr::E_ERROR;
        }
        size_t relationLen = 0;
        for (auto &database : databases) {
            auto it = dbRelation.relations.find(database.name);
            if (it == dbRelation.relations.end()) {
                continue;
            }
            uint32_t subId = std::stoul(it->second);
            if (OhCloudExtVectorPush(relation, &subId, sizeof(uint32_t)) != ERRNO_SUCCESS) {
                return DBErr::E_ERROR;
            }
            relationLen += 1;
        }
        auto status = OhCloudExtHashMapInsert(pSubs.get(),
            const_cast<void *>(reinterpret_cast<const void *>(bundle.c_str())), bundle.size(), relation, relationLen);
        if (status != ERRNO_SUCCESS) {
            return DBErr::E_ERROR;
        }
        bundles.emplace_back(bundle);
    }
    if (DoUnsubscribe(pServer, pSubs, bundles, sub) != DBErr::E_OK) {
        return DBErr::E_ERROR;
    }
    DBMetaMgr::GetInstance().SaveMeta(sub.GetKey(), sub, true);
    return DBErr::E_OK;
}

int32_t CloudServerImpl::DoUnsubscribe(std::shared_ptr<OhCloudExtCloudSync> server,
    std::shared_ptr<OhCloudExtHashMap> relations, const std::vector<std::string> &bundles, DBSub &sub)
{
    OhCloudExtVector *errs = nullptr;
    auto status =  OhCloudExtCloudSyncUnsubscribe(server.get(), relations.get(), &errs);
    if (status != ERRNO_SUCCESS) {
        return DBErr::E_ERROR;
    }
    if (errs != nullptr) {
        auto pErrs = std::shared_ptr<OhCloudExtVector>(errs, [](auto *errs) { OhCloudExtVectorFree(errs); });
        size_t errsLen = 0;
        OhCloudExtVectorGetLength(pErrs.get(), reinterpret_cast<unsigned int *>(&errsLen));
        if (errsLen != bundles.size()) {
            return DBErr::E_ERROR;
        }
        for (size_t i = 0; i < errsLen; i++) {
            void *value = nullptr;
            size_t valueLen = 0;
            status = OhCloudExtVectorGet(pErrs.get(), i, &value, reinterpret_cast<unsigned int *>(&valueLen));
            if (status != ERRNO_SUCCESS || value == nullptr) {
                return DBErr::E_ERROR;
            }
            int err = *reinterpret_cast<int *>(value);
            if (err != ERRNO_SUCCESS) {
                continue;
            }
            sub.expiresTime.erase(bundles[i]);
            DBMetaMgr::GetInstance().DelMeta(sub.GetRelationKey(bundles[i]), true);
        }
    } else {
        for (size_t i = 0; i < bundles.size(); i++) {
            sub.expiresTime.erase(bundles[i]);
            DBMetaMgr::GetInstance().DelMeta(sub.GetRelationKey(bundles[i]), true);
        }
    }
    return DBErr::E_OK;
}

std::shared_ptr<DBAssetLoader> CloudServerImpl::ConnectAssetLoader(uint32_t tokenId, const DBMeta &dbMeta)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return nullptr;
    }
    HapTokenInfo hapInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != RET_SUCCESS) {
        return nullptr;
    }
    auto data = ExtensionUtil::Convert(dbMeta);
    if (data.first == nullptr) {
        return nullptr;
    }
    OhCloudExtCloudAssetLoader *loader = OhCloudExtCloudAssetLoaderNew(hapInfo.userID,
        reinterpret_cast<const unsigned char *>(hapInfo.bundleName.c_str()),
        hapInfo.bundleName.size(), data.first);
    return loader != nullptr ? std::make_shared<AssetLoaderImpl>(loader) : nullptr;
}

std::shared_ptr<DBAssetLoader> CloudServerImpl::ConnectAssetLoader(
    const std::string &bundleName, int user, const DBMeta &dbMeta)
{
    auto data = ExtensionUtil::Convert(dbMeta);
    if (data.first == nullptr) {
        return nullptr;
    }
    OhCloudExtCloudAssetLoader *loader = OhCloudExtCloudAssetLoaderNew(
        user, reinterpret_cast<const unsigned char *>(bundleName.c_str()), bundleName.size(), data.first);
    return loader != nullptr ? std::make_shared<AssetLoaderImpl>(loader) : nullptr;
}

std::shared_ptr<DBCloudDB> CloudServerImpl::ConnectCloudDB(uint32_t tokenId, const DBMeta &dbMeta)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return nullptr;
    }
    HapTokenInfo hapInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != RET_SUCCESS) {
        return nullptr;
    }
    auto data = ExtensionUtil::Convert(dbMeta);
    if (data.first == nullptr) {
        return nullptr;
    }
    OhCloudExtCloudDatabase *cloudDb = OhCloudExtCloudDbNew(hapInfo.userID,
        reinterpret_cast<const unsigned char *>(hapInfo.bundleName.c_str()),
        hapInfo.bundleName.size(), data.first);
    return cloudDb != nullptr ? std::make_shared<CloudDbImpl>(cloudDb) : nullptr;
}

std::shared_ptr<DBCloudDB> CloudServerImpl::ConnectCloudDB(
    const std::string &bundleName, int user, const DBMeta &dbMeta)
{
    auto data = ExtensionUtil::Convert(dbMeta);
    if (data.first == nullptr) {
        return nullptr;
    }
    OhCloudExtCloudDatabase *cloudDb = OhCloudExtCloudDbNew(
        user, reinterpret_cast<const unsigned char *>(bundleName.c_str()), bundleName.size(), data.first);
    return cloudDb != nullptr ? std::make_shared<CloudDbImpl>(cloudDb) : nullptr;
}
} // namespace OHOS::CloudData
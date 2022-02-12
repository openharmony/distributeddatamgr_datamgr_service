/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef VIRTUAL_RELATIONAL_VER_SYNC_DB_INTERFACE_H
#define VIRTUAL_RELATIONAL_VER_SYNC_DB_INTERFACE_H
#ifdef RELATIONAL_STORE

#include "data_transformer.h"
#include "relational_db_sync_interface.h"
#include "sqlite_single_ver_continue_token.h"
#include "relational_schema_object.h"

namespace DistributedDB {
struct VirtualRowData {
    LogInfo logInfo;
    ObjectData objectData;
};

class VirtualRelationalVerSyncDBInterface : public RelationalDBSyncInterface {
public:
    VirtualRelationalVerSyncDBInterface() = default;
    ~VirtualRelationalVerSyncDBInterface() override = default;

    int PutSyncDataWithQuery(const QueryObject &query, const std::vector<SingleVerKvEntry *> &entries,
    const std::string &deviceName) override;

    int PutLocalData(const std::vector<VirtualRowData> &dataList, const std::string &tableName);

    RelationalSchemaObject GetSchemaInfo() const override;

    int GetDatabaseCreateTimeStamp(TimeStamp &outTime) const override;

    int GetBatchMetaData(const std::vector<Key> &keys, std::vector<Entry> &entries) const override;

    int PutBatchMetaData(std::vector<Entry> &entries) override;

    std::vector<QuerySyncObject> GetTablesQuery() override;

    int LocalDataChanged(int notifyEvent, std::vector<QuerySyncObject> &queryObj) override;

    int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
        const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
        std::vector<SingleVerKvEntry *> &entries) const override;

    int GetInterfaceType() const override;

    void IncRefCount() override;

    void DecRefCount() override;

    std::vector<uint8_t> GetIdentifier() const override;

    void GetMaxTimeStamp(TimeStamp &stamp) const override;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value) override;

    int DeleteMetaData(const std::vector<Key> &keys) override;

    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    const KvDBProperties &GetDbProperties() const override;

    void SetLocalFieldInfo(const std::vector<FieldInfo> &localFieldInfo);

    int GetAllSyncData(const std::string &tableName, std::vector<VirtualRowData> &data);

    int GetVirtualSyncData(const std::string &tableName, const std::string &hashKey, VirtualRowData &data);

    int InterceptData(std::vector<SingleVerKvEntry *> &entries,
        const std::string &sourceID, const std::string &targetID) const override
    {
        return E_OK;
    }

    int CheckAndInitQueryCondition(QueryObject &query) const override
    {
        return E_OK;
    }

    int CreateDistributedDeviceTable(const std::string &device, const RelationalSyncStrategy &syncStrategy) override;

    int RegisterSchemaChangedCallback(const std::function<void()> &onSchemaChanged) override;

    void EraseSyncData(const std::string &tableName);

    void SetTableInfo(const TableInfo &tableInfo);

private:

    mutable std::map<std::vector<uint8_t>, std::vector<uint8_t>> metadata_;
    std::map<std::string, std::map<std::string, VirtualRowData>> syncData_;
    mutable std::map<std::string, std::map<std::string, VirtualRowData>> localData_;
    std::string schema_;
    RelationalSchemaObject schemaObj_;
    std::vector<FieldInfo> localFieldInfo_;
    KvDBProperties properties_;
    SecurityOption secOption_;
};
}
#endif
#endif // VIRTUAL_RELATIONAL_VER_SYNC_DB_INTERFACE_H
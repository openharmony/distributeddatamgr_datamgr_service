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
#include "single_ver_relational_syncer.h"
#ifdef RELATIONAL_STORE
#include "db_common.h"
#include "relational_db_sync_interface.h"
#include "single_ver_sync_engine.h"

namespace DistributedDB {
int SingleVerRelationalSyncer::PrepareSync(const SyncParma &param, uint32_t syncId)
{
    const auto &syncInterface = static_cast<RelationalDBSyncInterface*>(syncInterface_);
    std::vector<QuerySyncObject> tablesQuery;
    if (param.isQuerySync) {
        tablesQuery.push_back(param.syncQuery);
    } else {
        tablesQuery = syncInterface->GetTablesQuery();
    }
    std::set<uint32_t> subSyncIdSet;
    int errCode = GenerateEachSyncTask(param, syncId, tablesQuery, subSyncIdSet);
    if (errCode != E_OK) {
        DoRollBack(subSyncIdSet);
        return errCode;
    }
    return E_OK;
}

int SingleVerRelationalSyncer::GenerateEachSyncTask(const SyncParma &param, uint32_t syncId,
    const std::vector<QuerySyncObject> &tablesQuery, std::set<uint32_t> &subSyncIdSet)
{
    SyncParma subParam = param;
    subParam.isQuerySync = true;
    int errCode = E_OK;
    for (const QuerySyncObject &table : tablesQuery) {
        uint32_t subSyncId = GenerateSyncId();
        LOGI("[SingleVerRelationalSyncer] SubSyncId %d create by SyncId %d, tableName = %s",
             subSyncId, syncId, table.GetRelationTableName().c_str());
        subParam.syncQuery = table;
        subParam.onComplete = std::bind(&SingleVerRelationalSyncer::DoOnSubSyncComplete, this, subSyncId,
            syncId, param, std::placeholders::_1);
        {
            std::lock_guard<std::mutex> lockGuard(syncMapLock_);
            fullSyncIdMap_[syncId].insert(subSyncId);
        }
        errCode = GenericSyncer::PrepareSync(subParam, subSyncId);
        if (errCode != E_OK) {
            LOGW("[SingleVerRelationalSyncer] PrepareSync failed errCode:%d", errCode);
            std::lock_guard<std::mutex> lockGuard(syncMapLock_);
            fullSyncIdMap_[syncId].erase(subSyncId);
            break;
        }
        subSyncIdSet.insert(subSyncId);
    }
    return errCode;
}

void SingleVerRelationalSyncer::DoOnSubSyncComplete(const uint32_t subSyncId, const uint32_t syncId,
    const SyncParma &param, const std::map<std::string, int> &devicesMap)
{
    static std::map<int, DBStatus> statusMap = {
        { static_cast<int>(SyncOperation::OP_FINISHED_ALL),                  OK },
        { static_cast<int>(SyncOperation::OP_TIMEOUT),                       TIME_OUT },
        { static_cast<int>(SyncOperation::OP_PERMISSION_CHECK_FAILED),       PERMISSION_CHECK_FORBID_SYNC },
        { static_cast<int>(SyncOperation::OP_COMM_ABNORMAL),                 COMM_FAILURE },
        { static_cast<int>(SyncOperation::OP_SECURITY_OPTION_CHECK_FAILURE), SECURITY_OPTION_CHECK_ERROR },
        { static_cast<int>(SyncOperation::OP_EKEYREVOKED_FAILURE),           EKEYREVOKED_ERROR },
        { static_cast<int>(SyncOperation::OP_SCHEMA_INCOMPATIBLE),           SCHEMA_MISMATCH },
        { static_cast<int>(SyncOperation::OP_BUSY_FAILURE),                  BUSY },
        { static_cast<int>(SyncOperation::OP_QUERY_FORMAT_FAILURE),          INVALID_QUERY_FORMAT },
        { static_cast<int>(SyncOperation::OP_QUERY_FIELD_FAILURE),           INVALID_QUERY_FIELD },
        { static_cast<int>(SyncOperation::OP_NOT_SUPPORT),                   NOT_SUPPORT },
    };
    bool allFinish = true;
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        fullSyncIdMap_[syncId].erase(subSyncId);
        allFinish = fullSyncIdMap_[syncId].empty();
        for (const auto &item : devicesMap) {
            DBStatus status = DB_ERROR;
            if (statusMap.find(item.second) != statusMap.end()) {
                status = statusMap[item.second];
            }
            resMap_[syncId][item.first][param.syncQuery.GetRelationTableName()] = status;
        }
    }
    if (allFinish) {
        DoOnComplete(param, syncId);
    }
}

void SingleVerRelationalSyncer::DoRollBack(std::set<uint32_t> &subSyncIdSet)
{
    for (const auto &removeId : subSyncIdSet) {
        int retCode = RemoveSyncOperation((int)removeId);
        if (retCode != E_OK) {
            LOGW("[SingleVerRelationalSyncer] RemoveSyncOperation failed errCode:%d, syncId:%d", retCode, removeId);
        }
    }
}

void SingleVerRelationalSyncer::DoOnComplete(const SyncParma &param, uint32_t syncId)
{
    if (!param.relationOnComplete) {
        return;
    }
    std::map<std::string, std::vector<TableStatus>> syncRes;
    std::map<std::string, std::map<std::string, int>> tmpMap;
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        tmpMap = resMap_[syncId];
    }
    for (const auto &devicesRes : tmpMap) {
        for (const auto &tableRes : devicesRes.second) {
            syncRes[devicesRes.first].push_back(
                {tableRes.first, static_cast<DBStatus>(tableRes.second)});
        }
    }
    param.relationOnComplete(syncRes);
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        resMap_.erase(syncId);
    }
}

void SingleVerRelationalSyncer::EnableAutoSync(bool enable)
{
}

int SingleVerRelationalSyncer::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    return E_OK;
}

void SingleVerRelationalSyncer::LocalDataChanged(int notifyEvent)
{
}

void SingleVerRelationalSyncer::RemoteDataChanged(const std::string &device)
{
}
}
#endif
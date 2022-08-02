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

#include "collaboration_log_table_manager.h"

namespace DistributedDB {
bool CollaborationLogTableManager::IsCollaborationWithoutKey(const TableInfo &table)
{
    return ((table.GetIdentifyKey().size() == 1u && table.GetIdentifyKey().at(0) == "rowid") ||
        table.GetAutoIncrement());
}

std::string CollaborationLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
    const std::string &identity)
{
    std::string sql;
    if (IsCollaborationWithoutKey(table)) {
        sql = "calc_hash('" + identity + "'||calc_hash(" + references + "rowid))";
    } else {
        if (table.GetIdentifyKey().size() == 1) {
            sql = "calc_hash(" + references + table.GetIdentifyKey().at(0)  + ")";
        }  else {
            sql = "calc_hash(";
            for (const auto &it : table.GetIdentifyKey()) {
                sql += "calc_hash(" + references + it + ")||";
            }
            sql.pop_back();
            sql.pop_back();
            sql += ")";
        }
    }
    return sql;
}

std::string CollaborationLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON " + table.GetTableName() + "\n";
    insertTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    insertTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key)";
    insertTrigger += " VALUES (new.rowid, '', '',";
    insertTrigger += " get_sys_time(0), get_sys_time(0),";
    insertTrigger += " CASE WHEN (SELECT count(*)<>0 FROM " + logTblName + " WHERE hash_key=" +
        CalcPrimaryKeyHash("NEW.", table, identity) + " AND flag&0x02=0x02) THEN 0x22 ELSE 0x02 END,";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ");\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string CollaborationLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON " + table.GetTableName() + "\n";
    updateTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    updateTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    updateTrigger += "BEGIN\n";
    if (table.GetIdentifyKey().size() == 1 && table.GetIdentifyKey().at(0) == "rowid") {
        updateTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
        updateTrigger += " SET timestamp=get_sys_time(0), device='', flag=0x22";
        updateTrigger += " WHERE data_key = OLD.rowid;";
    } else {
        updateTrigger += "\t UPDATE " + logTblName;
        updateTrigger += " SET data_key=-1, timestamp=get_sys_time(0), device='', flag=0x03";
        updateTrigger += " WHERE data_key = OLD.rowid;\n";
        updateTrigger += "\t INSERT OR REPLACE INTO " + logTblName + " VALUES (NEW.rowid, '', '', get_sys_time(0), "
            "get_sys_time(0), CASE WHEN (" + CalcPrimaryKeyHash("NEW.", table, identity) + " != " +
            CalcPrimaryKeyHash("NEW.", table, identity) + ") THEN 0x02 ELSE 0x22 END, " +
            CalcPrimaryKeyHash("NEW.", table, identity) + ");\n";
    }
    updateTrigger += "END;";
    return updateTrigger;
}

std::string CollaborationLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON " + table.GetTableName() + "\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_sys_time(0)";
    deleteTrigger += " WHERE data_key = OLD.rowid;";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::string CollaborationLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(hash_key)";
}

std::string CollaborationLogTableManager::GetIndexSql(const TableInfo &table)
{
    return SqliteLogTableManager::GetIndexSql(table) +
        "CREATE INDEX IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + "datakey_index ON " + GetLogTableName(table) +
        "(data_key);";
}
}
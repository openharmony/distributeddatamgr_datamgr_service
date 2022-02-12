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
#ifndef SQLITE_QUERY_HELPER_H
#define SQLITE_QUERY_HELPER_H

#include <list>
#include <set>
#include <string>
#include <vector>

#include "query_expression.h"
#include "schema_utils.h"
#include "sqlite_import.h"

namespace DistributedDB {
namespace TriggerMode {
enum class TriggerModeEnum;
}
struct QueryObjInfo {
    SchemaObject schema_;
    std::list<QueryObjNode> queryObjNodes_;
    std::vector<uint8_t> prefixKey_;
    std::string suggestIndex_;
    std::set<Key> keys_;
    int orderByCounts_ = 0; // Record processing to which orderBy node
    bool isValid_ = true;
    bool hasOrderBy_ = false;
    bool hasLimit_ = false;
    bool hasPrefixKey_ = false;
    std::string tableName_;
    bool isRelationalQuery_ = false;
};

enum SymbolType : uint32_t {
    INVALID_SYMBOL = 0x0000,
    COMPARE_SYMBOL = 0x0100, // relation symbol use to compare
    RELATIONAL_SYMBOL = 0x0200,
    RANGE_SYMBOL = 0x0300,
    PREFIXKEY_SYMBOL = 0x0400,
    LOGIC_SYMBOL = 0x0500,
    LINK_SYMBOL = 0x0600, // use to link relatonal symbol
    SPECIAL_SYMBOL = 0x0700, // need special precess and need at the last
    SUGGEST_INDEX_SYMBOL = 0x0800,
    IN_KEYS_SYMBOL = 0x0900,
};

class SqliteQueryHelper final {
public:
    explicit SqliteQueryHelper(const QueryObjInfo &info);

    // forbidden move constructor.
    SqliteQueryHelper(SqliteQueryHelper &&) = delete;
    SqliteQueryHelper &operator=(SqliteQueryHelper &&) = delete;
    // forbidden copy constructor.
    SqliteQueryHelper(const SqliteQueryHelper &) = delete;
    SqliteQueryHelper &operator=(const SqliteQueryHelper &) = delete;

    ~SqliteQueryHelper() = default;

    int GetQuerySqlStatement(sqlite3 *dbHandle, bool onlyRowid, sqlite3_stmt *&statement);
    int GetQuerySqlStatement(sqlite3 *dbHandle, const std::string &sql, sqlite3_stmt *&statement);
    int GetCountSqlStatement(sqlite3 *dbHandle, sqlite3_stmt *&countStmt);

    // For query Sync
    int GetQuerySyncStatement(sqlite3 *dbHandle, uint64_t beginTime, uint64_t endTime, sqlite3_stmt *&statement);
    int GetSyncDataCheckSql(std::string &sql);
    int BindSyncDataCheckStmt(sqlite3_stmt *statement, const Key &hashKey) const;

    int GetSubscribeSql(const std::string &subscribeId, TriggerMode::TriggerModeEnum mode,
        std::string &subscribeCondition);

    static SymbolType GetSymbolType(const QueryObjType &queryObjType);

    // public for unit test
    int GetQuerySql(std::string &sql, bool onlyRowid);
    int GetCountQuerySql(std::string &sql);

    const std::string &GetTableName()
    {
        return tableName_;
    }

    int GetRelationalSyncDataQuerySql(std::string &sql, bool hasSubQuery);
    int GetRelationalQueryStatement(sqlite3 *dbHandle, uint64_t beginTime, uint64_t endTime, sqlite3_stmt *&statement);

private:
    int ToQuerySql();
    int ToQuerySyncSql(bool hasSubQuery, bool useTimeStampAlias = false);
    int ToGetCountSql();
    int ParseQueryExpression(const QueryObjNode &queryNode, std::string &querySql,
        const std::string &accessStr = "", bool placeholder = true);
    std::string MapRelationalSymbolToSql(const QueryObjNode &queryNode, bool placeholder = false) const;
    std::string MapKeywordSymbolToSql(const QueryObjNode &queryNode);
    std::string MapLogicSymbolToSql(const QueryObjNode &queryNode) const;
    std::string MapValueToSql(const QueryObjNode &queryNode, bool placeholder) const;
    std::string MapCastFuncSql(const QueryObjNode &queryNode, const std::string &accessStr = "");
    std::string MapCastTypeSql(const FieldType &type) const;
    int BindFieldValue(sqlite3_stmt *statement, const QueryObjNode &queryNode, int &index) const;
    bool FilterSymbolToAddBracketLink(std::string &querySql, bool isNeedLink = true) const;
    std::string AssembleSqlForSuggestIndex(const std::string &baseSql, const std::string &filter) const;
    std::string CheckAndFormatSuggestIndex() const;
    int GetSyncDataQuerySql(std::string &sql, bool hasSubQuery);
    int ParseQueryObjNodeToSQL();
    int BindTimeRange(sqlite3_stmt *&statement, int &index, uint64_t beginTime, uint64_t endTime) const;
    int BindObjNodes(sqlite3_stmt *&statement, int &index) const;
    int GetSubscribeCondition(const std::string &accessStr, std::string &conditionStr);
    std::string MapKeysInToSql(size_t keysNum) const;
    int BindKeysToStmt(std::set<Key> &keys, sqlite3_stmt *&countStmt, int &index) const;

    std::string MapKeysInSubCondition(const std::string &accessStr) const;  // For InKeys.
    // Return the left string of symbol in compare clause.
    std::string GetFieldShape(const QueryObjNode &queryNode, const std::string &accessStr = "");

    SchemaObject schema_;
    std::list<QueryObjNode> queryObjNodes_;
    std::vector<uint8_t> prefixKey_;
    std::string suggestIndex_;
    std::string tableName_;
    std::set<Key> keys_;

    std::string querySql_;
    std::string countSql_;

    int orderByCounts_; // Record processing to which orderBy node
    bool isValid_;
    bool transformed_;
    bool hasOrderBy_;
    bool hasLimit_;
    bool isOrderByAppeared_;
    bool hasPrefixKey_;
    bool isNeedOrderbyKey_;  // The tag field is used for prefix query filtering key sorting
    bool isRelationalQuery_;
};
}
#endif
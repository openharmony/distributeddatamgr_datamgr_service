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
#ifndef QUERY_OBJECT_H
#define QUERY_OBJECT_H

#include <string>
#include "schema_object.h"
#include "query.h"
#include "sqlite_query_helper.h"

namespace DistributedDB {
class QueryObject {
public:
    QueryObject();
    explicit QueryObject(const Query &query);
    // for query sync
    QueryObject(const std::list<QueryObjNode> &queryObjNodes, const std::vector<uint8_t> &prefixKey);
    virtual ~QueryObject();
    int Init();
    SqliteQueryHelper GetQueryHelper(int &errCode);

    // suggest: get those attributes after init or GetQueryHelper to parsed query
    bool IsValid();
    bool HasLimit() const;
    void GetLimitVal(int &limit, int &offset) const;
    bool IsCountValid() const;

    const std::vector<uint8_t> &GetPrefixKey() const;
    void SetSchema(const SchemaObject &schema);

    bool IsQueryOnlyByKey() const;

    void SetTableName(const std::string &tableName)
    {
        tableName_ = tableName;
        isTableNameSpecified_ = true;
    }

    const std::string &GetTableName() const
    {
        return tableName_;
    }

    bool HasOrderBy() const;

    int ParseQueryObjNodes();

    bool Empty() const;

protected:
    std::list<QueryObjNode> queryObjNodes_;
    std::vector<uint8_t> prefixKey_;
    std::string tableName_ = "sync_data";
    std::string suggestIndex_;

    bool isValid_ = true;

    bool initialized_ = false; // use function need after init
    bool isTableNameSpecified_ = false;

private:
    int Parse();
    int ParseNode(const std::list<QueryObjNode>::iterator &iter);
    int CheckEqualFormat(const std::list<QueryObjNode>::iterator &iter) const;
    int CheckLinkerFormat(const std::list<QueryObjNode>::iterator &iter) const;
    int CheckSuggestIndexFormat(const std::list<QueryObjNode>::iterator &iter) const;
    int CheckOrderByFormat(const std::list<QueryObjNode>::iterator &iter);
    int CheckLimitFormat(const std::list<QueryObjNode>::iterator &iter) const;
    int CheckLinkerBefore(const std::list<QueryObjNode>::iterator &iter) const;
    void ClearNodesFlag();
    void GetAttrFromQueryObjNodes();

    SchemaObject schema_; // used to check and parse schema filed
    int limit_;
    int offset_;
    bool hasOrderBy_;
    bool hasLimit_;
    bool hasPrefixKey_;
    int orderByCounts_;
};
}
#endif

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

#ifndef DISTRIBUTEDDB_QUERY_H
#define DISTRIBUTEDDB_QUERY_H

#include <string>
#include <vector>
#include <list>

#include "query_expression.h"
#include "types_export.h"

namespace DistributedDB {
class GetQueryInfo;
class Query {
public:

    // Do not support concurrent use of query objects
    DB_API static Query Select();
    DB_API static Query Select(const std::string &tableName);

    template<typename T>
    DB_API Query &EqualTo(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::EQUALTO, field, type, fieldValue);
        return *this;
    }

    template<typename T>
    DB_API Query &NotEqualTo(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::NOT_EQUALTO, field, type, fieldValue);
        return *this;
    }

    template<typename T>
    DB_API Query &GreaterThan(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::GREATER_THAN, field, type, fieldValue);
        return *this;
    }

    template<typename T>
    DB_API Query &LessThan(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::LESS_THAN, field, type, fieldValue);
        return *this;
    }

    template<typename T>
    DB_API Query &GreaterThanOrEqualTo(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::GREATER_THAN_OR_EQUALTO, field, type, fieldValue);
        return *this;
    }

    template<typename T>
    DB_API Query &LessThanOrEqualTo(const std::string &field, const T &value)
    {
        FieldValue fieldValue;
        QueryValueType type = GetFieldTypeAndValue(value, fieldValue);
        ExecuteCompareOperation(QueryObjType::LESS_THAN_OR_EQUALTO, field, type, fieldValue);
        return *this;
    }

    DB_API Query &OrderBy(const std::string &field, bool isAsc = true);

    DB_API Query &Limit(int number, int offset = 0);

    DB_API Query &Like(const std::string &field, const std::string &value);

    DB_API Query &NotLike(const std::string &field, const std::string &value);

    template<typename T>
    DB_API Query &In(const std::string &field, const std::vector<T> &values)
    {
        std::vector<FieldValue> fieldValues;
        QueryValueType type;
        for (const auto &value : values) {
            FieldValue fieldValue;
            type = GetFieldTypeAndValue(value, fieldValue);
            fieldValues.push_back(fieldValue);
        }

        ExecuteCompareOperation(QueryObjType::IN, field, type, fieldValues);
        return *this;
    }

    template<typename T>
    DB_API Query &NotIn(const std::string &field, const std::vector<T> &values)
    {
        std::vector<FieldValue> fieldValues;
        QueryValueType type;
        for (const auto &value : values) {
            FieldValue fieldValue;
            type = GetFieldTypeAndValue(value, fieldValue);
            fieldValues.push_back(fieldValue);
        }

        ExecuteCompareOperation(QueryObjType::NOT_IN, field, type, fieldValues);
        return *this;
    }

    DB_API Query &IsNull(const std::string &field);

    DB_API Query &And();

    DB_API Query &Or();

    DB_API Query &IsNotNull(const std::string &field);

    DB_API Query &BeginGroup();

    DB_API Query &EndGroup();

    DB_API Query &PrefixKey(const std::vector<uint8_t> &key);

    DB_API Query &SuggestIndex(const std::string &indexName);

    friend class GetQueryInfo;
    ~Query() = default;

private:
    Query() = default;
    Query(const std::string &tableName);

    DB_SYMBOL void ExecuteCompareOperation(QueryObjType operType, const std::string &field,
        const QueryValueType type, const FieldValue &fieldValue);
    DB_SYMBOL void ExecuteCompareOperation(QueryObjType operType, const std::string &field,
        const QueryValueType type, const std::vector<FieldValue> &fieldValue);

    template<typename T>
    QueryValueType GetFieldTypeAndValue(const T &queryValue, FieldValue &fieldValue)
    {
        return GetQueryValueType::GetFieldTypeAndValue(queryValue, fieldValue);
    }

    QueryExpression queryExpression_;
};
} // namespace DistributedDB
#endif // DISTRIBUTEDDB_QUERY_H

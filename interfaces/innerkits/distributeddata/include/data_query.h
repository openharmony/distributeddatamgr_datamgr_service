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

#ifndef DISTRIBUTED_DATA_QUERY_H
#define DISTRIBUTED_DATA_QUERY_H

#include <string>
#include <vector>
#include <sstream>
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
class DataQuery {
public:
    API_EXPORT DataQuery();

    API_EXPORT ~DataQuery() = default;

    // Reset the query.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &Reset();

    // Equal to int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EqualTo(const std::string &field, const int value);

    // Equal to long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EqualTo(const std::string &field, const int64_t value);

    // Equal to double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EqualTo(const std::string &field, const double value);

    // Equal to String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EqualTo(const std::string &field, const std::string &value);

    // Equal to boolean value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EqualTo(const std::string &field, const bool value);

    // Not equal to int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotEqualTo(const std::string &field, const int value);

    // Not equal to long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotEqualTo(const std::string &field, const int64_t value);

    // Not equal to double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotEqualTo(const std::string &field, const double value);

    // Not equal to String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotEqualTo(const std::string &field, const std::string &value);

    // Not equal to boolean value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotEqualTo(const std::string &field, const bool value);

    // Greater than int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThan(const std::string &field, const int value);

    // Greater than long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThan(const std::string &field, const int64_t value);

    // Greater than double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThan(const std::string &field, const double value);

    // Greater than String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThan(const std::string &field, const std::string &value);

    // Less than int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThan(const std::string &field, const int value);

    // Less than long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThan(const std::string &field, const int64_t value);

    // Less than double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThan(const std::string &field, const double value);

    // Less than String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThan(const std::string &field, const std::string &value);

    // Greater than or equal to int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThanOrEqualTo(const std::string &field, const int value);

    // Greater than or equal to long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThanOrEqualTo(const std::string &field, const int64_t value);

    // Greater than or equal to double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThanOrEqualTo(const std::string &field, const double value);

    // Greater than or equal to String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &GreaterThanOrEqualTo(const std::string &field, const std::string &value);

    // Less than or equal to int value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThanOrEqualTo(const std::string &field, const int value);

    // Less than or equal to long value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThanOrEqualTo(const std::string &field, const int64_t value);

    // Less than or equal to double value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThanOrEqualTo(const std::string &field, const double value);

    // Less than or equal to String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &LessThanOrEqualTo(const std::string &field, const std::string &value);

    // Is null field value.
    // Parameters:
    //     field: the field name.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &IsNull(const std::string &field);

    // Is not null field value.
    // Parameters:
    //     field: the field name.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &IsNotNull(const std::string &field);

    // In int value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &In(const std::string &field, const std::vector<int> &valueList);

    // In long value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &In(const std::string &field, const std::vector<int64_t> &valueList);

    // In Double value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &In(const std::string &field, const std::vector<double> &valueList);

    // In String value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &In(const std::string &field, const std::vector<std::string> &valueList);

    // Not in int value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotIn(const std::string &field, const std::vector<int> &valueList);

    // Not in long value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotIn(const std::string &field, const std::vector<int64_t> &valueList);

    // Not in Double value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotIn(const std::string &field, const std::vector<double> &valueList);

    // Not in String value list.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &NotIn(const std::string &field, const std::vector<std::string> &valueList);

    // Like String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &Like(const std::string &field, const std::string &value);

    // Unlike String value.
    // Parameters:
    //     field: the field name.
    //     value: the field value list.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &Unlike(const std::string &field, const std::string &value);

    // And operator.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &And();

    // Or operator.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &Or();

    // Order by ascent.
    // Parameters:
    //     field: the field name.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &OrderByAsc(const std::string &field);

    // Order by descent.
    // Parameters:
    //     field: the field name.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &OrderByDesc(const std::string &field);

    // Limit result size.
    // Parameters:
    //     number: the number of results.
    //     offset: the start position.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &Limit(const int number, const int offset);

    // Begin group.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &BeginGroup();

    // End group.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &EndGroup();

    // Select results with specified key prefix.
    // Parameters:
    //     prefix: key prefix.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &KeyPrefix(const std::string &prefix);

    // Select results with specified device Identifier.
    // Parameters:
    //     deviceId: device Identifier.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &DeviceId(const std::string &deviceId);

    // Select results with suggested index.
    // Parameters:
    //     index: suggested index.
    // Return:
    //     This Query.
    API_EXPORT DataQuery &SetSuggestIndex(const std::string &index);

    // Select results with many keys.
    // Parameters:
    //     keys: the vector of keys for query
    // Return:
    //     This Query.
    API_EXPORT DataQuery &InKeys(const std::vector<std::string> &keys);

    // Get string representation
    // Return:
    //     String representation of this query.
    API_EXPORT std::string ToString() const;

private:

    friend class QueryHelper;
    // equal to
    static const char * const EQUAL_TO;

    // not equal to
    static const char * const NOT_EQUAL_TO;

    // greater than
    static const char * const GREATER_THAN;

    // less than
    static const char * const LESS_THAN;

    // greater than or equal to
    static const char * const GREATER_THAN_OR_EQUAL_TO;

    // less than or equal to
    static const char * const LESS_THAN_OR_EQUAL_TO;

    // is null
    static const char * const IS_NULL;

    // in
    static const char * const IN;

    // not in
    static const char * const NOT_IN;

    // like
    static const char * const LIKE;

    // not like
    static const char * const NOT_LIKE;

    // and
    static const char * const AND;

    // or
    static const char * const OR;

    // order by asc
    static const char * const ORDER_BY_ASC;

    // order by desc
    static const char * const ORDER_BY_DESC;

    // limit
    static const char * const LIMIT;

    // space
    static const char * const SPACE;

    // '^'
    static const char * const SPECIAL;

    // '^' escape
    static const char * const SPECIAL_ESCAPE;

    // space escape
    static const char * const SPACE_ESCAPE;

    // empty string
    static const char * const EMPTY_STRING;

    // start in
    static const char * const START_IN;

    // end in
    static const char * const END_IN;

    // begin group
    static const char * const BEGIN_GROUP;

    // end group
    static const char * const END_GROUP;

    // key prefix
    static const char * const KEY_PREFIX;

    // device id
    static const char * const DEVICE_ID;

    // is not null
    static const char * const IS_NOT_NULL;

    // type string
    static const char * const TYPE_STRING;

    // type integer
    static const char * const TYPE_INTEGER;

    // type long
    static const char * const TYPE_LONG;

    // type double
    static const char * const TYPE_DOUBLE;

    // type boolean
    static const char * const TYPE_BOOLEAN;

    // value true
    static const char * const VALUE_TRUE;

    // value false
    static const char * const VALUE_FALSE;

    // suggested index
    static const char * const SUGGEST_INDEX;

    // in keys
    static const char * const IN_KEYS;

    std::string str_;

    bool inkeysFlag_ = false;

    template<typename T>
    void AppendCommon(const std::string &keyword, const std::string &fieldType, std::string &field, const T &value);

    void AppendCommonString(const std::string &keyword, const std::string &fieldType, std::string &field,
                            std::string &value);

    void AppendCommonBoolean(const std::string &keyword, const std::string &fieldType, std::string &field,
                             const bool &value);

    void AppendCommonString(const std::string &keyword, std::string &field, std::string &value);

    template<typename T>
    void AppendCommonList(const std::string &keyword, const std::string &fieldType, std::string &field,
                          const std::vector<T> &valueList);

    void AppendCommonListString(const std::string &keyword, const std::string &fieldType, std::string &field,
                                std::vector<std::string> &valueList);

    void EscapeSpace(std::string &input);

    bool ValidateField(const std::string &field);

    template<typename T>
    std::string BasicToString(const T &value);
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // DISTRIBUTED_DATA_QUERY_H

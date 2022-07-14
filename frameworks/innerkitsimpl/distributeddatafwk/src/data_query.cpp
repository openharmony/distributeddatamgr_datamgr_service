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

#define LOG_TAG "DataQuery"

#include "data_query.h"
#include "log_print.h"
#include "query.h"
namespace OHOS {
namespace DistributedKv {
const char * const DataQuery::EQUAL_TO = "^EQUAL";
const char * const DataQuery::NOT_EQUAL_TO = "^NOT_EQUAL";
const char * const DataQuery::GREATER_THAN = "^GREATER";
const char * const DataQuery::LESS_THAN = "^LESS";
const char * const DataQuery::GREATER_THAN_OR_EQUAL_TO = "^GREATER_EQUAL";
const char * const DataQuery::LESS_THAN_OR_EQUAL_TO = "^LESS_EQUAL";
const char * const DataQuery::IS_NULL = "^IS_NULL";
const char * const DataQuery::IN = "^IN";
const char * const DataQuery::NOT_IN = "^NOT_IN";
const char * const DataQuery::LIKE = "^LIKE";
const char * const DataQuery::NOT_LIKE = "^NOT_LIKE";
const char * const DataQuery::AND = "^AND";
const char * const DataQuery::OR = "^OR";
const char * const DataQuery::ORDER_BY_ASC = "^ASC";
const char * const DataQuery::ORDER_BY_DESC = "^DESC";
const char * const DataQuery::LIMIT = "^LIMIT";
const char * const DataQuery::SPACE = " ";
const char * const DataQuery::SPECIAL = "^";
const char * const DataQuery::SPECIAL_ESCAPE = "(^)";
const char * const DataQuery::SPACE_ESCAPE = "^^";
const char * const DataQuery::EMPTY_STRING = "^EMPTY_STRING";
const char * const DataQuery::START_IN = "^START";
const char * const DataQuery::END_IN = "^END";
const char * const DataQuery::BEGIN_GROUP = "^BEGIN_GROUP";
const char * const DataQuery::END_GROUP = "^END_GROUP";
const char * const DataQuery::KEY_PREFIX = "^KEY_PREFIX";
const char * const DataQuery::DEVICE_ID = "^DEVICE_ID";
const char * const DataQuery::IS_NOT_NULL = "^IS_NOT_NULL";
const char * const DataQuery::TYPE_STRING = "STRING";
const char * const DataQuery::TYPE_INTEGER = "INTEGER";
const char * const DataQuery::TYPE_LONG = "LONG";
const char * const DataQuery::TYPE_DOUBLE = "DOUBLE";
const char * const DataQuery::TYPE_BOOLEAN = "BOOL";
const char * const DataQuery::VALUE_TRUE = "true";
const char * const DataQuery::VALUE_FALSE = "false";
const char * const DataQuery::SUGGEST_INDEX = "^SUGGEST_INDEX";
const char * const DataQuery::IN_KEYS = "^IN_KEYS";
constexpr int MAX_QUERY_LENGTH = 5 * 1024; // Max query string length 5k

DataQuery::DataQuery()
{
    query_ = std::make_shared<DistributedDB::Query>();
}

DataQuery& DataQuery::Reset()
{
    str_ = "";
    hasKeys_ = false;
    hasPrefix_ = false;
    deviceId_ = "";
    prefix_ = "";
    query_ = std::make_shared<DistributedDB::Query>();
    return *this;
}

DataQuery& DataQuery::EqualTo(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(EQUAL_TO, TYPE_INTEGER, myField, value);
        query_->EqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::EqualTo(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(EQUAL_TO, TYPE_LONG, myField, value);
        query_->EqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::EqualTo(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(EQUAL_TO, TYPE_DOUBLE, myField, value);
        query_->EqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::EqualTo(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(EQUAL_TO, TYPE_STRING, myField, myValue);
        query_->EqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::EqualTo(const std::string &field, const bool value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonBoolean(EQUAL_TO, TYPE_BOOLEAN, myField, value);
        query_->EqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::NotEqualTo(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(NOT_EQUAL_TO, TYPE_INTEGER, myField, value);
        query_->NotEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::NotEqualTo(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(NOT_EQUAL_TO, TYPE_LONG, myField, value);
        query_->NotEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::NotEqualTo(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(NOT_EQUAL_TO, TYPE_DOUBLE, myField, value);
        query_->NotEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::NotEqualTo(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(NOT_EQUAL_TO, TYPE_STRING, myField, myValue);
        query_->NotEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::NotEqualTo(const std::string &field, const bool value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonBoolean(NOT_EQUAL_TO, TYPE_BOOLEAN, myField, value);
        query_->NotEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThan(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN, TYPE_INTEGER, myField, value);
        query_->GreaterThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThan(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN, TYPE_LONG, myField, value);
        query_->GreaterThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThan(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN, TYPE_DOUBLE, myField, value);
        query_->GreaterThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThan(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(GREATER_THAN, TYPE_STRING, myField, myValue);
        query_->GreaterThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThan(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN, TYPE_INTEGER, myField, value);
        query_->LessThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThan(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN, TYPE_LONG, myField, value);
        query_->LessThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThan(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN, TYPE_DOUBLE, myField, value);
        query_->LessThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThan(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(LESS_THAN, TYPE_STRING, myField, myValue);
        query_->LessThan(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThanOrEqualTo(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN_OR_EQUAL_TO, TYPE_INTEGER, myField, value);
        query_->GreaterThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThanOrEqualTo(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN_OR_EQUAL_TO, TYPE_LONG, myField, value);
        query_->GreaterThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThanOrEqualTo(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(GREATER_THAN_OR_EQUAL_TO, TYPE_DOUBLE, myField, value);
        query_->GreaterThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::GreaterThanOrEqualTo(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(GREATER_THAN_OR_EQUAL_TO, TYPE_STRING, myField, myValue);
        query_->GreaterThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThanOrEqualTo(const std::string &field, const int value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN_OR_EQUAL_TO, TYPE_INTEGER, myField, value);
        query_->LessThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThanOrEqualTo(const std::string &field, const int64_t value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN_OR_EQUAL_TO, TYPE_LONG, myField, value);
        query_->LessThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThanOrEqualTo(const std::string &field, const double value)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommon(LESS_THAN_OR_EQUAL_TO, TYPE_DOUBLE, myField, value);
        query_->LessThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::LessThanOrEqualTo(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(LESS_THAN_OR_EQUAL_TO, TYPE_STRING, myField, myValue);
        query_->LessThanOrEqualTo(field, value);
    }
    return *this;
}

DataQuery& DataQuery::IsNull(const std::string &field)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        str_.append(SPACE);
        str_.append(IS_NULL);
        str_.append(SPACE);
        EscapeSpace(myField);
        str_.append(myField);
        query_->IsNull(field);
    }
    return *this;
}

DataQuery& DataQuery::IsNotNull(const std::string &field)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        str_.append(SPACE);
        str_.append(IS_NOT_NULL);
        str_.append(SPACE);
        EscapeSpace(myField);
        str_.append(myField);
        query_->IsNotNull(field);
    }
    return *this;
}

DataQuery& DataQuery::In(const std::string &field, const std::vector<int> &valueList)
{
    ZLOGD("DataQuery::In int");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(IN, TYPE_INTEGER, myField, valueList);
        query_->In(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::In(const std::string &field, const std::vector<int64_t> &valueList)
{
    ZLOGD("DataQuery::In int64_t");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(IN, TYPE_LONG, myField, valueList);
        query_->In(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::In(const std::string &field, const std::vector<double> &valueList)
{
    ZLOGD("DataQuery::In double");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(IN, TYPE_DOUBLE, myField, valueList);
        query_->In(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::In(const std::string &field, const std::vector<std::string> &valueList)
{
    ZLOGD("DataQuery::In string");
    std::string myField = field;
    std::vector<std::string> myValueList(valueList);
    if (ValidateField(myField)) {
        AppendCommonListString(IN, TYPE_STRING, myField, myValueList);
        query_->In(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::NotIn(const std::string &field, const std::vector<int> &valueList)
{
    ZLOGD("DataQuery::NotIn int");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(NOT_IN, TYPE_INTEGER, myField, valueList);
        query_->NotIn(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::NotIn(const std::string &field, const std::vector<int64_t> &valueList)
{
    ZLOGD("DataQuery::NotIn int64_t");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(NOT_IN, TYPE_LONG, myField, valueList);
        query_->NotIn(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::NotIn(const std::string &field, const std::vector<double> &valueList)
{
    ZLOGD("DataQuery::NotIn double");
    std::string myField = field;
    if (ValidateField(myField)) {
        AppendCommonList(NOT_IN, TYPE_DOUBLE, myField, valueList);
        query_->NotIn(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::NotIn(const std::string &field, const std::vector<std::string> &valueList)
{
    ZLOGD("DataQuery::NotIn string");
    std::string myField = field;
    std::vector<std::string> myValueList(valueList);
    if (ValidateField(myField)) {
        AppendCommonListString(NOT_IN, TYPE_STRING, myField, myValueList);
        query_->NotIn(field, valueList);
    }
    return *this;
}

DataQuery& DataQuery::Like(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(LIKE, myField, myValue);
        query_->Like(field, value);
    }
    return *this;
}

DataQuery& DataQuery::Unlike(const std::string &field, const std::string &value)
{
    std::string myField = field;
    std::string myValue = value;
    if (ValidateField(myField)) {
        AppendCommonString(NOT_LIKE, myField, myValue);
        query_->NotLike(field, value);
    }
    return *this;
}

DataQuery& DataQuery::And()
{
    str_.append(SPACE);
    str_.append(AND);
    query_->And();
    return *this;
}

DataQuery& DataQuery::Or()
{
    str_.append(SPACE);
    str_.append(OR);
    query_->Or();
    return *this;
}

DataQuery& DataQuery::OrderByAsc(const std::string &field)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        str_.append(SPACE);
        str_.append(ORDER_BY_ASC);
        str_.append(SPACE);
        EscapeSpace(myField);
        str_.append(myField);
        query_->OrderBy(field);
    }
    return *this;
}

DataQuery& DataQuery::OrderByDesc(const std::string &field)
{
    std::string myField = field;
    if (ValidateField(myField)) {
        str_.append(SPACE);
        str_.append(ORDER_BY_DESC);
        str_.append(SPACE);
        EscapeSpace(myField);
        str_.append(myField);
        query_->OrderBy(field, false);
    }
    return *this;
}

DataQuery& DataQuery::Limit(const int number, const int offset)
{
    if (number < 0 || offset < 0) {
        ZLOGE("Invalid number param");
        return *this;
    }
    str_.append(SPACE);
    str_.append(LIMIT);
    str_.append(SPACE);
    str_.append(BasicToString(number));
    str_.append(SPACE);
    str_.append(BasicToString(offset));
    query_->Limit(number, offset);
    return *this;
}

DataQuery& DataQuery::BeginGroup()
{
    str_.append(SPACE);
    str_.append(BEGIN_GROUP);
    query_->BeginGroup();
    return *this;
}

DataQuery& DataQuery::EndGroup()
{
    str_.append(SPACE);
    str_.append(END_GROUP);
    query_->EndGroup();
    return *this;
}

DataQuery& DataQuery::KeyPrefix(const std::string &prefix)
{
    std::string myPrefix = prefix;
    if (ValidateField(myPrefix)) {
        str_.append(SPACE);
        str_.append(KEY_PREFIX);
        str_.append(SPACE);
        EscapeSpace(myPrefix);
        str_.append(myPrefix);
        prefix_ = prefix;
        hasPrefix_ = true;
    }
    return *this;
}

DataQuery& DataQuery::DeviceId(const std::string &deviceId)
{
    std::string device = deviceId;
    if (ValidateField(device)) {
        std::string start;
        start.append(SPACE);
        start.append(DEVICE_ID);
        start.append(SPACE);
        EscapeSpace(device);
        start.append(device);
        str_ = start + str_; // start with diveceId
        deviceId_ = deviceId;
    }
    return *this;
}

DataQuery& DataQuery::SetSuggestIndex(const std::string &index)
{
    std::string suggestIndex = index;
    if (ValidateField(suggestIndex)) {
        str_.append(SPACE);
        str_.append(SUGGEST_INDEX);
        str_.append(SPACE);
        EscapeSpace(suggestIndex);
        str_.append(suggestIndex);
        query_->SuggestIndex(index);
    }
    return *this;
}

DataQuery& DataQuery::InKeys(const std::vector<std::string> &keys)
{
    if (keys.empty()) {
        ZLOGE("Invalid number param");
        return *this;
    }
    if (hasKeys_) {
        ZLOGE("cannot set inkeys more than once");
        return *this;
    }
    hasKeys_ = true;
    str_.append(SPACE);
    str_.append(IN_KEYS);
    str_.append(SPACE);
    str_.append(START_IN);
    str_.append(SPACE);
    for (std::string key : keys) {
        if (ValidateField(key)) {
            EscapeSpace(key);
            str_.append(key);
            str_.append(SPACE);
        }
    }
    str_.append(END_IN);
    keys_ = keys;
    return *this;
}

std::string DataQuery::ToString() const
{
    if (str_.length() > MAX_QUERY_LENGTH) {
        ZLOGE("Query is too long");
        return std::string();
    }
    std::string str(str_.begin(), str_.end());
    return str;
}

template<typename T>
void DataQuery::AppendCommon(const std::string &keyword, const std::string &fieldType,
                             std::string &field, const T &value)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    str_.append(fieldType);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    str_.append(BasicToString(value));
}

void DataQuery::AppendCommonString(const std::string &keyword, const std::string &fieldType,
                                   std::string &field, std::string &value)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    str_.append(fieldType);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    EscapeSpace(value);
    str_.append(value);
}

void DataQuery::AppendCommonBoolean(const std::string &keyword, const std::string &fieldType,
                                    std::string &field, const bool &value)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    str_.append(fieldType);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    if (value) {
        str_.append(VALUE_TRUE);
    } else {
        str_.append(VALUE_FALSE);
    }
}

void DataQuery::AppendCommonString(const std::string &keyword, std::string &field, std::string &value)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    EscapeSpace(value);
    str_.append(value);
}

template<typename T>
void DataQuery::AppendCommonList(const std::string &keyword, const std::string &fieldType,
                                 std::string &field, const std::vector<T> &valueList)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    str_.append(fieldType);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    str_.append(START_IN);
    str_.append(SPACE);
    for (T object : valueList) {
        str_.append(BasicToString(object));
        str_.append(SPACE);
    }
    str_.append(END_IN);
}

void DataQuery::AppendCommonListString(const std::string &keyword, const std::string &fieldType,
                                       std::string &field, std::vector<std::string> &valueList)
{
    str_.append(SPACE);
    str_.append(keyword);
    str_.append(SPACE);
    str_.append(fieldType);
    str_.append(SPACE);
    EscapeSpace(field);
    str_.append(field);
    str_.append(SPACE);
    str_.append(START_IN);
    str_.append(SPACE);
    for (std::string str : valueList) {
        EscapeSpace(str);
        str_.append(str);
        str_.append(SPACE);
    }
    str_.append(END_IN);
}

void DataQuery::EscapeSpace(std::string &input)
{
    if (input.length() == 0) {
        input = EMPTY_STRING;
    }
    size_t index = 0; // search from the beginning of the string
    while (true) {
        index = input.find(DataQuery::SPECIAL, index);
        if (index == std::string::npos) {
            break;
        }
        input.replace(index, 1, DataQuery::SPECIAL_ESCAPE); // 1 char to be replaced
        index += 3; // replaced with 3 chars, keep searching the remaining string
    }
    index = 0; // search from the beginning of the string
    while (true) {
        index = input.find(DataQuery::SPACE, index);
        if (index == std::string::npos) {
            break;
        }
        input.replace(index, 1, DataQuery::SPACE_ESCAPE); // 1 char to be replaced
        index += 2; // replaced with 2 chars, keep searching the remaining string
    }
}

bool DataQuery::ValidateField(const std::string &field)
{
    if (field.empty() || field.find(DataQuery::SPECIAL) != std::string::npos) {
        ZLOGE("invalid string argument");
        return false;
    }
    return true;
}

template<typename T>
std::string DataQuery::BasicToString(const T &value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}
}  // namespace DistributedKv
}  // namespace OHOS

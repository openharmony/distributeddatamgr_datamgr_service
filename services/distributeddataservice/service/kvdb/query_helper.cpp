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
#define LOG_TAG "QueryHelper"
#include "query_helper.h"
#include <regex>
#include <sstream>
#include "data_query.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "types.h"
namespace OHOS::DistributedKv {
constexpr int QUERY_SKIP_SIZE = 1;
constexpr int QUERY_WORD_SIZE = 2;
constexpr int MAX_QUERY_LENGTH = 5 * 1024; // Max query string length 5k
constexpr int MAX_QUERY_COMPLEXITY = 500;  // Max query complexity 500
bool QueryHelper::hasPrefixKey_ = false;
std::string QueryHelper::deviceId_;

DistributedDB::Query QueryHelper::StringToDbQuery(const std::string &query, bool &isSuccess)
{
    ZLOGI("query string length:%{public}zu", query.length());
    DBQuery dbQuery = DBQuery::Select();
    if (query.empty()) {
        ZLOGI("Query string is empty.");
        isSuccess = true;
        return dbQuery;
    }
    if (query.size() > MAX_QUERY_LENGTH) {
        ZLOGE("Query string is too long.");
        isSuccess = false;
        return dbQuery;
    }
    deviceId_.clear();
    hasPrefixKey_ = (query.find(DataQuery::KEY_PREFIX) != std::string::npos);
    size_t pos = query.find_first_not_of(DataQuery::SPACE);
    std::string inputTrim = (pos == std::string::npos) ? "" : query.substr(pos);
    std::regex regex(" ");
    std::vector<std::string> words(
        std::sregex_token_iterator(inputTrim.begin(), inputTrim.end(), regex, -1), // regex split string by space
        std::sregex_token_iterator());

    int pointer = 0;            // Read pointer starts at 0
    int end = words.size() - 1; // Read pointer ends at size - 1
    int count = 0;              // Counts how many keywords has been handled
    while (pointer <= end && count <= MAX_QUERY_COMPLEXITY) {
        count++;
        std::string keyword = words.at(pointer);
        if (keyword == DataQuery::EQUAL_TO) {
            isSuccess = HandleEqualTo(words, pointer, end, dbQuery);
        } else if (keyword == DataQuery::NOT_EQUAL_TO) {
            isSuccess = HandleNotEqualTo(words, pointer, end, dbQuery);
        } else if (keyword == DataQuery::GREATER_THAN) {
            isSuccess = HandleGreaterThan(words, pointer, end, dbQuery);
        } else if (keyword == DataQuery::LESS_THAN) {
            isSuccess = HandleLessThan(words, pointer, end, dbQuery);
        } else if (keyword == DataQuery::GREATER_THAN_OR_EQUAL_TO) {
            isSuccess = HandleGreaterThanOrEqualTo(words, pointer, end, dbQuery);
        } else if (keyword == DataQuery::LESS_THAN_OR_EQUAL_TO) {
            isSuccess = HandleLessThanOrEqualTo(words, pointer, end, dbQuery);
        } else {
            isSuccess = Handle(words, pointer, end, dbQuery);
        }
        if (!isSuccess) {
            ZLOGE("Invalid params.");
            return DBQuery::Select();
        }
    }
    return dbQuery;
}

bool QueryHelper::Handle(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    std::string keyword = words.at(pointer);
    if (keyword == DataQuery::IS_NULL) {
        return HandleIsNull(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::IN) {
        return HandleIn(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::NOT_IN) {
        return HandleNotIn(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::LIKE) {
        return HandleLike(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::NOT_LIKE) {
        return HandleNotLike(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::AND) {
        return HandleAnd(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::OR) {
        return HandleOr(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::ORDER_BY_ASC) {
        return HandleOrderByAsc(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::ORDER_BY_DESC) {
        return HandleOrderByDesc(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::ORDER_BY_WRITE_TIME) {
        return HandleOrderByWriteTime(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::LIMIT) {
        return HandleLimit(words, pointer, end, dbQuery);
    } else {
        return HandleExtra(words, pointer, end, dbQuery);
    }
}

bool QueryHelper::HandleExtra(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    std::string keyword = words.at(pointer);
    if (keyword == DataQuery::BEGIN_GROUP) {
        return HandleBeginGroup(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::END_GROUP) {
        return HandleEndGroup(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::KEY_PREFIX) {
        return HandleKeyPrefix(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::IS_NOT_NULL) {
        return HandleIsNotNull(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::DEVICE_ID) {
        return HandleDeviceId(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::SUGGEST_INDEX) {
        return HandleSetSuggestIndex(words, pointer, end, dbQuery);
    } else if (keyword == DataQuery::IN_KEYS) {
        return HandleInKeys(words, pointer, end, dbQuery);
    }
    ZLOGE("Invalid keyword.");
    return false;
}

bool QueryHelper::HandleEqualTo(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("EqualTo not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.EqualTo(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.EqualTo(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.EqualTo(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_BOOLEAN) {
        dbQuery.EqualTo(StringToString(fieldName), StringToBoolean(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.EqualTo(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("EqualTo wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleNotEqualTo(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("NotEqualTo not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.NotEqualTo(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.NotEqualTo(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.NotEqualTo(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_BOOLEAN) {
        dbQuery.NotEqualTo(StringToString(fieldName), StringToBoolean(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.NotEqualTo(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("NotEqualTo wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleGreaterThan(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("GreaterThan not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.GreaterThan(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.GreaterThan(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.GreaterThan(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.GreaterThan(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("GreaterThan wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleLessThan(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("LessThan not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.LessThan(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.LessThan(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.LessThan(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.LessThan(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("LessThan wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleGreaterThanOrEqualTo(
    const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("GreaterThanOrEqualTo not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.GreaterThanOrEqualTo(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.GreaterThanOrEqualTo(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.GreaterThanOrEqualTo(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.GreaterThanOrEqualTo(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("GreaterThanOrEqualTo wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleLessThanOrEqualTo(
    const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 3 > end) { // This keyword has 3 following params
        ZLOGE("LessThanOrEqualTo not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1);  // fieldType
    const std::string &fieldName = words.at(pointer + 2);  // fieldName
    const std::string &fieldValue = words.at(pointer + 3); // fieldValue
    if (fieldType == DataQuery::TYPE_INTEGER) {
        dbQuery.LessThanOrEqualTo(StringToString(fieldName), StringToInt(fieldValue));
    } else if (fieldType == DataQuery::TYPE_LONG) {
        dbQuery.LessThanOrEqualTo(StringToString(fieldName), StringToLong(fieldValue));
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        dbQuery.LessThanOrEqualTo(StringToString(fieldName), StringToDouble(fieldValue));
    } else if (fieldType == DataQuery::TYPE_STRING) {
        dbQuery.LessThanOrEqualTo(StringToString(fieldName), StringToString(fieldValue));
    } else {
        ZLOGE("LessThanOrEqualTo wrong type.");
        return false;
    }
    pointer += 4; // 4 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleIsNull(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("IsNull not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1); // fieldName
    dbQuery.IsNull(StringToString(fieldName));
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleIsNotNull(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("IsNotNull not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1); // fieldName
    dbQuery.IsNotNull(StringToString(fieldName));
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleIn(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    //       | <-------------------------4---------------------------->|
    // words [ DataQuery::IN, fieldType, fieldName, DataQuery::START_IN, ...valueList, DataQuery::END_IN ]
    // index [ -------0-----, ----1----, ----2----, ---------3---------,      ...    , ---------n--------]
    //                ^                                                                                  ^
    //                |                                                                                  |
    //              pointer                                                                             end
    if (pointer + 4 > end || words.at(pointer + 3) != DataQuery::START_IN) {
        ZLOGE("In not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1); // fieldType
    const std::string &fieldName = words.at(pointer + 2); // fieldName
    int elementPointer = pointer + 4;                     // first fieldValue, or END if list is empty
    if (fieldType == DataQuery::TYPE_INTEGER) {
        const std::vector<int> intValueList = GetIntegerList(words, elementPointer, end);
        dbQuery.In(StringToString(fieldName), intValueList);
    } else if (fieldType == DataQuery::TYPE_LONG) {
        const std::vector<int64_t> longValueList = GetLongList(words, elementPointer, end);
        dbQuery.In(StringToString(fieldName), longValueList);
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        const std::vector<double> doubleValueList = GetDoubleList(words, elementPointer, end);
        dbQuery.In(StringToString(fieldName), doubleValueList);
    } else if (fieldType == DataQuery::TYPE_STRING) {
        const std::vector<std::string> stringValueList = GetStringList(words, elementPointer, end);
        dbQuery.In(StringToString(fieldName), stringValueList);
    } else {
        ZLOGE("In wrong type.");
        return false;
    }
    pointer = elementPointer + 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleNotIn(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    //       |<--------------------------4-------------------------------->|
    // words [ DataQuery::NOT_IN, fieldType, fieldName, DataQuery::START_IN, ...valueList, DataQuery::END_IN ]
    // index [ --------0--------, ----1----, ----2----, ---------3---------,      ...    , ---------n--------]
    //                 ^                                                                                     ^
    //                 |                                                                                     |
    //               pointer                                                                                end
    if (pointer + 4 > end || words.at(pointer + 3) != DataQuery::START_IN) {
        ZLOGE("NotIn not enough params.");
        return false;
    }
    const std::string &fieldType = words.at(pointer + 1); // fieldType
    const std::string &fieldName = words.at(pointer + 2); // fieldName
    int elementPointer = pointer + 4;                     // first fieldValue, or END if list is empty
    if (fieldType == DataQuery::TYPE_INTEGER) {
        const std::vector<int> intValueList = GetIntegerList(words, elementPointer, end);
        dbQuery.NotIn(StringToString(fieldName), intValueList);
    } else if (fieldType == DataQuery::TYPE_LONG) {
        const std::vector<int64_t> longValueList = GetLongList(words, elementPointer, end);
        dbQuery.NotIn(StringToString(fieldName), longValueList);
    } else if (fieldType == DataQuery::TYPE_DOUBLE) {
        const std::vector<double> doubleValueList = GetDoubleList(words, elementPointer, end);
        dbQuery.NotIn(StringToString(fieldName), doubleValueList);
    } else if (fieldType == DataQuery::TYPE_STRING) {
        const std::vector<std::string> stringValueList = GetStringList(words, elementPointer, end);
        dbQuery.NotIn(StringToString(fieldName), stringValueList);
    } else {
        ZLOGE("NotIn wrong type.");
        return false;
    }
    pointer = elementPointer + 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleLike(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 2 > end) { // This keyword has 2 following params
        ZLOGE("Like not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1);  // fieldName
    const std::string &fieldValue = words.at(pointer + 2); // fieldValue
    dbQuery.Like(StringToString(fieldName), StringToString(fieldValue));
    pointer += 3; // 3 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleNotLike(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 2 > end) { // This keyword has 2 following params
        ZLOGE("NotLike not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1);  // fieldName
    const std::string &fieldValue = words.at(pointer + 2); // fieldValue
    dbQuery.NotLike(StringToString(fieldName), StringToString(fieldValue));
    pointer += 3; // 3 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleAnd(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    dbQuery.And();
    pointer += 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleOr(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    dbQuery.Or();
    pointer += 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleOrderByAsc(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("OrderByAsc not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1); // fieldName
    dbQuery.OrderBy(StringToString(fieldName), true);
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleOrderByDesc(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("OrderByDesc not enough params.");
        return false;
    }
    const std::string &fieldName = words.at(pointer + 1); // fieldName
    dbQuery.OrderBy(StringToString(fieldName), false);
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleOrderByWriteTime(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("HandleOrderByWriteTime not enough params.");
        return false;
    }
    const std::string isAsc = words.at(pointer + 1); // isASC

    dbQuery.OrderByWriteTime(isAsc == DataQuery::IS_ASC);
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleLimit(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 2 > end) { // This keyword has 2 following params
        ZLOGE("Limit not enough params.");
        return false;
    }
    const int number = StringToInt(words.at(pointer + 1)); // number
    const int offset = StringToInt(words.at(pointer + 2)); // offset
    dbQuery.Limit(number, offset);
    pointer += 3; // 3 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleBeginGroup(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    dbQuery.BeginGroup();
    pointer += 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleEndGroup(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    dbQuery.EndGroup();
    pointer += 1; // Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleKeyPrefix(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("KeyPrefix not enough params.");
        return false;
    }
    const std::string &prefix = deviceId_ + StringToString(words.at(pointer + 1)); // prefix
    const std::vector<uint8_t> prefixVector(prefix.begin(), prefix.end());
    dbQuery.PrefixKey(prefixVector);
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleInKeys(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    // pointer points at keyword "IN_KEYS", (pointer + 1) points at keyword "START_IN"
    int startInOffSet = pointer + 1;
    int queryLen = end - pointer;
    if (queryLen < 2 || words.at(startInOffSet) != DataQuery::START_IN) { // This keyword has at least 2 params
        ZLOGE("In not enough params.");
        return false;
    }
    int inkeyOffSet = startInOffSet + 1; // inkeyOffSet points at the first inkey value
    const std::vector<std::string> inKeys = GetStringList(words, inkeyOffSet, end);
    std::set<std::vector<uint8_t>> inDbKeys;
    for (const std::string &inKey : inKeys) {
        ZLOGI("inKey=%{public}s", inKey.c_str());
        std::vector<uint8_t> dbKey;
        dbKey.assign(inKey.begin(), inKey.end());
        inDbKeys.insert(dbKey);
    }
    int size = inDbKeys.size();
    ZLOGI("size of inKeys=%{public}d", size);
    dbQuery.InKeys(inDbKeys);
    int endOffSet = inkeyOffSet;
    pointer = endOffSet + 1; // endOffSet points at keyword "END", Pointer goes to next keyword
    return true;
}

bool QueryHelper::HandleSetSuggestIndex(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + QUERY_SKIP_SIZE > end) {
        ZLOGE("HandleSetSuggestIndex not enough params.");
        return false;
    }
    std::string index = StringToString(words.at(pointer + QUERY_SKIP_SIZE));
    dbQuery.SuggestIndex(index);
    pointer += QUERY_WORD_SIZE;
    return true;
}

bool QueryHelper::HandleDeviceId(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery)
{
    if (pointer + 1 > end) { // This keyword has 1 following params
        ZLOGE("DeviceId not enough params.");
        return false;
    }
    deviceId_ = StringToString(words.at(pointer + 1)); // deviceId
    ZLOGI("query devId string length:%zu", deviceId_.length());
    deviceId_ = AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(deviceId_); // convert to UUId
    ZLOGI("query converted devId string length:%zu", deviceId_.length());
    if (!hasPrefixKey_) {
        ZLOGD("DeviceId as the only prefixKey.");
        const std::vector<uint8_t> prefixVector(deviceId_.begin(), deviceId_.end());
        dbQuery.PrefixKey(prefixVector);
    } else {
        ZLOGD("Join deviceId with user specified prefixkey later.");
    }
    pointer += 2; // 2 Pointer goes to next keyword
    return true;
}

int QueryHelper::StringToInt(const std::string &word)
{
    int result;
    std::istringstream(word) >> result;
    return result;
}

int64_t QueryHelper::StringToLong(const std::string &word)
{
    int64_t result;
    std::istringstream(word) >> result;
    return result;
}

double QueryHelper::StringToDouble(const std::string &word)
{
    double result;
    std::istringstream(word) >> result;
    return result;
}

bool QueryHelper::StringToBoolean(const std::string &word)
{
    if (word == DataQuery::VALUE_TRUE) {
        return true;
    } else if (word == DataQuery::VALUE_FALSE) {
        return false;
    } else {
        ZLOGE("StringToBoolean wrong value.");
        return false;
    }
}

std::string QueryHelper::StringToString(const std::string &word)
{
    std::string result = word;
    if (result.compare(DataQuery::EMPTY_STRING) == 0) {
        result = "";
        return result;
    }
    size_t index = 0; // search from the beginning of the string
    while (true) {
        index = result.find(DataQuery::SPACE_ESCAPE, index);
        if (index == std::string::npos) {
            break;
        }
        result.replace(index, 2, DataQuery::SPACE); // 2 chars to be replaced
        index += 1;                                 // replaced with 1 char, keep searching the remaining string
    }
    index = 0; // search from the beginning of the string
    while (true) {
        index = result.find(DataQuery::SPECIAL_ESCAPE, index);
        if (index == std::string::npos) {
            break;
        }
        result.replace(index, 3, DataQuery::SPECIAL); // 3 chars to be replaced
        index += 1;                                   // replaced with 1 char, keep searching the remaining string
    }
    return result;
}

std::vector<int> QueryHelper::GetIntegerList(const std::vector<std::string> &words, int &elementPointer, int end)
{
    std::vector<int> valueList;
    bool isEndFound = false;
    while (elementPointer <= end) {
        if (words.at(elementPointer) == DataQuery::END_IN) {
            isEndFound = true;
            break;
        }
        valueList.push_back(StringToInt(words.at(elementPointer)));
        elementPointer++;
    }
    if (isEndFound) {
        return valueList;
    } else {
        ZLOGE("GetIntegerList failed.");
        return std::vector<int>();
    }
}

std::vector<int64_t> QueryHelper::GetLongList(const std::vector<std::string> &words, int &elementPointer, int end)
{
    std::vector<int64_t> valueList;
    bool isEndFound = false;
    while (elementPointer <= end) {
        if (words.at(elementPointer) == DataQuery::END_IN) {
            isEndFound = true;
            break;
        }
        valueList.push_back(StringToLong(words.at(elementPointer)));
        elementPointer++;
    }
    if (isEndFound) {
        return valueList;
    } else {
        ZLOGE("GetLongList failed.");
        return std::vector<int64_t>();
    }
}

std::vector<double> QueryHelper::GetDoubleList(const std::vector<std::string> &words, int &elementPointer, int end)
{
    std::vector<double> valueList;
    bool isEndFound = false;
    while (elementPointer <= end) {
        if (words.at(elementPointer) == DataQuery::END_IN) {
            isEndFound = true;
            break;
        }
        valueList.push_back(StringToDouble(words.at(elementPointer)));
        elementPointer++;
    }
    if (isEndFound) {
        return valueList;
    } else {
        ZLOGE("GetDoubleList failed.");
        return std::vector<double>();
    }
}

std::vector<std::string> QueryHelper::GetStringList(const std::vector<std::string> &words, int &elementPointer, int end)
{
    std::vector<std::string> valueList;
    bool isEndFound = false;
    while (elementPointer <= end) {
        if (words.at(elementPointer) == DataQuery::END_IN) {
            isEndFound = true;
            break;
        }
        valueList.push_back(StringToString(words.at(elementPointer)));
        elementPointer++;
    }
    if (isEndFound) {
        return valueList;
    } else {
        ZLOGE("GetStringList failed.");
        return std::vector<std::string>();
    }
}
} // namespace OHOS::DistributedKv

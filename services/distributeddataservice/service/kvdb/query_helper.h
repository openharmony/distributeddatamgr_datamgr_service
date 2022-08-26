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

#ifndef QUERY_HELPER_H
#define QUERY_HELPER_H

#include <set>

#include "query.h"
#include "types.h"

namespace OHOS::DistributedKv {
class QueryHelper {
public:
    API_EXPORT static DistributedDB::Query StringToDbQuery(const std::string &query, bool &isSuccess);

private:
    using DBQuery = DistributedDB::Query;
    static std::string deviceId_;
    static bool hasPrefixKey_;
    static bool hasInKeys_;
    static bool Handle(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleExtra(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleEqualTo(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleNotEqualTo(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleGreaterThan(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleLessThan(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleGreaterThanOrEqualTo(
        const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleLessThanOrEqualTo(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleIsNull(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleIsNotNull(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleIn(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleNotIn(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleLike(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleNotLike(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleAnd(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleOr(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleOrderByAsc(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleOrderByDesc(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleOrderByWriteTime(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleLimit(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleBeginGroup(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleEndGroup(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleKeyPrefix(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleInKeys(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleSetSuggestIndex(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static bool HandleDeviceId(const std::vector<std::string> &words, int &pointer, int end, DBQuery &dbQuery);
    static int StringToInt(const std::string &word);
    static int64_t StringToLong(const std::string &word);
    static double StringToDouble(const std::string &word);
    static bool StringToBoolean(const std::string &word);
    static std::string StringToString(const std::string &word);
    static std::vector<int> GetIntegerList(const std::vector<std::string> &words, int &elementPointer, int end);
    static std::vector<int64_t> GetLongList(const std::vector<std::string> &words, int &elementPointer, int end);
    static std::vector<double> GetDoubleList(const std::vector<std::string> &words, int &elementPointer, int end);
    static std::vector<std::string> GetStringList(const std::vector<std::string> &words, int &elementPointer, int end);
};
} // namespace OHOS::DistributedKv
#endif // QUERY_HELPER_H

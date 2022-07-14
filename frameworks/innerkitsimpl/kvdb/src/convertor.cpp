/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "convertor.h"
namespace OHOS::DistributedKv {
std::vector<uint8_t> Convertor::ToLocalDBKey(const Key &key) const
{
    return Convertor::GetPrefix(key);
}

std::vector<uint8_t> Convertor::ToWholeDBKey(const Key &key) const
{
    return Convertor::GetPrefix(key);
}

Key Convertor::ToKey(DBKey &&key, std::string &deviceId) const
{
    return std::move(key);
}

std::vector<uint8_t> Convertor::GetPrefix(const Key &prefix) const
{
    std::vector<uint8_t> dbKey = TrimKey(prefix);
    if (dbKey.size() > MAX_KEY_LENGTH) {
        dbKey.clear();
    }
    return dbKey;
}

std::vector<uint8_t> Convertor::GetPrefix(const DataQuery &query) const
{
    return Convertor::GetPrefix(Key(query.prefix_));
}

Convertor::DBQuery Convertor::GetDBQuery(const DataQuery &query) const
{
    DBQuery dbQuery = *(query.query_);
    if (query.hasPrefix_) {
        dbQuery.PrefixKey(GetPrefix(query));
    }

    if (query.hasKeys_) {
        std::set<DBKey> keys;
        for (auto &key : query.keys_) {
            keys.insert(ToWholeDBKey(GetRealKey(key, query)));
        }
        dbQuery.InKeys(keys);
    }
    return dbQuery;
}

std::string Convertor::GetRealKey(const std::string &key, const DataQuery &query) const
{
    (void)query;
    return key;
}

std::vector<uint8_t> Convertor::TrimKey(const Key &prefix) const
{
    auto begin = std::find_if(prefix.Data().begin(), prefix.Data().end(), [](int ch) { return !std::isspace(ch); });
    auto rBegin = std::find_if(prefix.Data().rbegin(), prefix.Data().rend(), [](int ch) { return !std::isspace(ch); });
    auto end = static_cast<decltype(begin)>(rBegin.base());
    if (end <= begin) {
        return {};
    }
    return {begin, end};
}
}
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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KEY_CONVERTOR_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KEY_CONVERTOR_H
#include <vector>
#include "data_query.h"
#include "query.h"
#include "types.h"
#include "types_export.h"
namespace OHOS::DistributedKv {
class Convertor {
public:
    using DBKey = DistributedDB::Key;
    using DBQuery = DistributedDB::Query;
    virtual std::vector<uint8_t> ToLocalDBKey(const Key &key) const;
    virtual std::vector<uint8_t> ToWholeDBKey(const Key &key) const;
    virtual Key ToKey(DBKey &&key, std::string &deviceId) const;
    virtual std::vector<uint8_t> GetPrefix(const Key &prefix) const;
    virtual std::vector<uint8_t> GetPrefix(const DataQuery &query) const;
    DBQuery GetDBQuery(const DataQuery &query) const;

protected:
    virtual std::string GetRealKey(const std::string &key, const DataQuery &query) const;
    std::vector<uint8_t> TrimKey(const Key &prefix) const;

private:
    static constexpr size_t MAX_KEY_LENGTH = 1024;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_KEY_CONVERTOR_H

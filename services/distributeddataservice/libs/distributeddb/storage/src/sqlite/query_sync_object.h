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

#ifndef QUERY_SYNC_OBJECT_H
#define QUERY_SYNC_OBJECT_H

#include <string>

#include "query_object.h"
#include "parcel.h"

namespace DistributedDB {
const uint32_t QUERY_SYNC_OBJECT_VERSION_0 = 0;
const uint32_t QUERY_SYNC_OBJECT_VERSION_1 = 1; // Add tableName_ and keys_.
const uint32_t QUERY_SYNC_OBJECT_VERSION_CURRENT = QUERY_SYNC_OBJECT_VERSION_1; // always point the last.

struct ObjContext {
    uint32_t version = QUERY_SYNC_OBJECT_VERSION_0; // serialized struct version
    std::vector<uint8_t> prefixKey{};
    std::string suggestIndex{};
    std::list<QueryObjNode> queryObjNodes{};
    std::vector<Key> keys{};
};

class QuerySyncObject : public QueryObject {
public:
    QuerySyncObject();
    QuerySyncObject(const std::list<QueryObjNode> &queryObjNodes, const std::vector<uint8_t> &prefixKey,
        const std::set<Key> &keys);
    explicit QuerySyncObject(const Query &query);
    ~QuerySyncObject() override;

    std::string GetIdentify() const;

    int SerializeData(Parcel &parcel, uint32_t softWareVersion);
    // should call Parcel.IsError() to Get result.
    static int DeSerializeData(Parcel &parcel, QuerySyncObject &queryObj);
    uint32_t CalculateParcelLen(uint32_t softWareVersion) const;

    std::string GetRelationTableName() const;

private:
    uint32_t CalculateLen() const;
    int GetObjContext(ObjContext &objContext) const;
    uint32_t GetVersion() const;
};
}
#endif
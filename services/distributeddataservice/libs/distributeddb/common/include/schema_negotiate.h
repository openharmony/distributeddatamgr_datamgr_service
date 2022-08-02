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
#ifndef SCHEMA_NEGOTIATE_H
#define SCHEMA_NEGOTIATE_H

#include "schema_object.h"
#include "relational_schema_object.h"

namespace DistributedDB {
struct SyncOpinion {
    bool permitSync = false;
    bool requirePeerConvert = false;
    bool checkOnReceive = false;
};

struct SyncStrategy {
    bool permitSync = false;
    bool convertOnSend = false;
    bool convertOnReceive = false;
    bool checkOnReceive = false;
};

using RelationalSyncOpinion = std::map<std::string, SyncOpinion>;
using RelationalSyncStrategy = std::map<std::string, SyncStrategy>;

class SchemaNegotiate {
public:
    // The remoteSchemaType may beyond local SchemaType definition
    static SyncOpinion MakeLocalSyncOpinion(const SchemaObject &localSchema, const std::string &remoteSchema,
        uint8_t remoteSchemaType);

    // The remoteOpinion.checkOnReceive is ignored
    static SyncStrategy ConcludeSyncStrategy(const SyncOpinion &localOpinion, const SyncOpinion &remoteOpinion);

    static RelationalSyncOpinion MakeLocalSyncOpinion(const RelationalSchemaObject &localSchema,
        const std::string &remoteSchema, uint8_t remoteSchemaType);

    // The remoteOpinion.checkOnReceive is ignored
    static RelationalSyncStrategy ConcludeSyncStrategy(const RelationalSyncOpinion &localOpinion,
        const RelationalSyncOpinion &remoteOpinion);

    static uint32_t CalculateParcelLen(const RelationalSyncOpinion &opinions);
    static int SerializeData(const RelationalSyncOpinion &opinions, Parcel &parcel);
    static int DeserializeData(Parcel &parcel, RelationalSyncOpinion &opinion);
private:
    SchemaNegotiate() = default;
    ~SchemaNegotiate() = default;

    static RelationalSyncOpinion MakeOpinionEachTable(const RelationalSchemaObject &localSchema,
        const RelationalSchemaObject &remoteSchema);
};
}

#endif // SCHEMA_NEGOTIATE_H
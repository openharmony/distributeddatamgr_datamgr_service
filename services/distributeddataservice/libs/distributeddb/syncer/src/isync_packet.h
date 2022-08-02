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

#ifndef ISYNC_PACKET_H
#define ISYNC_PACKET_H

#include "sync_types.h"

namespace DistributedDB {
class ISyncPacket {
public:
    virtual ~ISyncPacket() {}

    virtual uint32_t CalculateLen() const = 0;

    virtual int Serialization(Parcel &parcel) const = 0;

    virtual int DeSerialization(Parcel &parcel) = 0;
};
}
#endif // ISYNC_PACKET_H
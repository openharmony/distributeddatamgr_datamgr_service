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

#ifndef INTERCEPTED_DATA_H
#define INTERCEPTED_DATA_H

#include <vector>
#include "types.h"
#include "types_export.h"

namespace DistributedDB {
struct KVEntry {
    const Key &key;
    const Value &value;
};

class InterceptedData {
public:
    InterceptedData() {}
    DB_API virtual ~InterceptedData() {}

    // Interface for getting the intercepted entries.
    DB_API virtual std::vector<KVEntry> GetEntries() = 0;

    // Interface for modifying key. Index is the index of GetEntries().
    DB_API virtual DBStatus ModifyKey(size_t index, const Key &newKey) = 0;

    // Interface for modifying value. Index is the index of GetEntries().
    DB_API virtual DBStatus ModifyValue(size_t index, const Value &newValue) = 0;
};

// The callback function works on the send data from device "sourceID" to device "targetID".
using PushDataInterceptor = std::function<int(InterceptedData &data, const std::string &sourceID,
    const std::string &targetID)>;
} // namespace DistributedDB
#endif // INTERCEPTED_DATA_H
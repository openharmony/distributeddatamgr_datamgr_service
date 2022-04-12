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
#ifndef SYNC_CONFIG_H
#define SYNC_CONFIG_H

#include <cstdint>
#include <set>
#include <map>
#include "macro_utils.h"
#include "parcel.h"
#include "types_export.h"

// db ability config
namespace DistributedDB {
// offset, used_bits_num, used_bits_num < 64
using AbilityItem = std::pair<uint32_t, uint32_t>;
// format: {offset, used_bits_num}
/*
if need to add new ability, just add append to the last ability
current ability format:
|first bit|second bit|third bit|
|DATABASE_COMPRESSION_ZLIB|ALLPREDICATEQUERY|SUBSCRIBEQUERY|
*/
class SyncConfig final {
public:
    static const AbilityItem DATABASE_COMPRESSION_ZLIB;
    static const AbilityItem ALLPREDICATEQUERY;
    static const AbilityItem SUBSCRIBEQUERY;
    static const AbilityItem INKEYS_QUERY;
    static const std::vector<AbilityItem> ABILITYBITS;
    static const std::map<const uint8_t, const AbilityItem> COMPRESSALGOMAP;
};
}
#endif
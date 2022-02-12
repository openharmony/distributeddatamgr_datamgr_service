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
#include "sync_config.h"
namespace DistributedDB {
const AbilityItem SyncConfig::DATABASE_COMPRESSION_ZLIB = {0, 1};
const AbilityItem SyncConfig::ALLPREDICATEQUERY = {1, 1}; // 0b10 {1: start at second bit, 1: 1 bit len}
const AbilityItem SyncConfig::SUBSCRIBEQUERY = {2, 1}; //   0b100
const AbilityItem SyncConfig::INKEYS_QUERY = {3, 1}; //    0b1000

const std::vector<AbilityItem> SyncConfig::ABILITYBITS = {
    DATABASE_COMPRESSION_ZLIB,
    ALLPREDICATEQUERY,
    SUBSCRIBEQUERY,
    INKEYS_QUERY};

const std::map<const uint8_t, const AbilityItem> SyncConfig::COMPRESSALGOMAP = {
    {static_cast<uint8_t>(CompressAlgorithm::ZLIB), DATABASE_COMPRESSION_ZLIB},
};
} // DistributedDB
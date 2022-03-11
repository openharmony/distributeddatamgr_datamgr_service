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

#ifndef DB_ABILITY_H
#define DB_ABILITY_H

#include <cstdint>
#include <set>
#include <map>
#include "macro_utils.h"
#include "parcel.h"
#include "sync_config.h"

namespace DistributedDB {
class DbAbility {
public:
    DbAbility();
    DbAbility(const DbAbility &other);
    DbAbility& operator=(const DbAbility &other);
    ~DbAbility() {};

    bool operator==(const DbAbility &other) const;
    // translate dbAbility_ to std::vector<uint64_t>
    static int Serialize(Parcel &parcel, const DbAbility &curAbility);

    static int DeSerialize(Parcel &parcel, DbAbility &curAbility);

    static uint32_t CalculateLen(const DbAbility &curAbility);

    void SetDbAbilityBuff(std::vector<bool> &buff);

    const std::vector<bool> &GetDbAbilityBuff() const;

    uint32_t GetAbilityBitsLen() const;

    uint8_t GetAbilityItem(const AbilityItem &abilityType) const;

    int SetAbilityItem(const AbilityItem &abilityType, uint8_t data);
private:
    constexpr static int SERIALIZE_BIT_SIZE = 64; // uint64_t bit size

    std::vector<bool> dbAbility_;
    std::set<AbilityItem> dbAbilityItemSet_;
};
} // namespace DistributedDB

#endif // DB_ABILITY_H
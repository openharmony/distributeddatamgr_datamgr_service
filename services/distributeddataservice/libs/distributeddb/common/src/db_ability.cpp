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

#include "db_ability.h"

#include <cmath>
#include "db_errno.h"
#include "types_export.h"

namespace DistributedDB {
DbAbility::DbAbility()
{
    for (const auto &item : SyncConfig::ABILITYBITS) {
        dbAbilityItemSet_.insert(item);
    }
    dbAbility_.resize(SyncConfig::ABILITYBITS.back().first + SyncConfig::ABILITYBITS.back().second);
}

DbAbility::DbAbility(const DbAbility &other)
{
    if (&other != this) {
        dbAbility_ = other.dbAbility_;
        dbAbilityItemSet_ = other.dbAbilityItemSet_;
    }
}

DbAbility& DbAbility::operator=(const DbAbility &other)
{
    if (&other != this) {
        dbAbility_ = other.dbAbility_;
        dbAbilityItemSet_ = other.dbAbilityItemSet_;
    }
    return *this;
}

bool DbAbility::operator==(const DbAbility &other) const
{
    return (dbAbility_ == other.dbAbility_) && (dbAbilityItemSet_ == other.dbAbilityItemSet_);
}

int DbAbility::Serialize(Parcel &parcel, const DbAbility &curAbility)
{
    uint32_t div = curAbility.GetAbilityBitsLen() / SERIALIZE_BIT_SIZE;
    uint32_t buffLen = (curAbility.GetAbilityBitsLen() % SERIALIZE_BIT_SIZE) ? div + 1 : div;
    std::vector<uint64_t> dstBuf(buffLen, 0);
    uint32_t buffOffset = 0;
    uint32_t innerBuffOffset = 0;
    const std::vector<bool> &abilityBuff = curAbility.GetDbAbilityBuff();
    for (uint32_t pos = 0; pos < curAbility.GetAbilityBitsLen(); pos++, innerBuffOffset++) {
        if (innerBuffOffset >= SERIALIZE_BIT_SIZE) {
            innerBuffOffset = 0;
            buffOffset++;
        }
        uint64_t value = static_cast<uint64_t>(abilityBuff[pos]) << innerBuffOffset;
        dstBuf[buffOffset] = dstBuf[buffOffset] | value;
    }
    int errCode = parcel.WriteVector<uint64_t>(dstBuf);
    if (errCode != E_OK) {
        return errCode;
    }
    return E_OK;
}

int DbAbility::DeSerialize(Parcel &parcel, DbAbility &curAbility)
{
    if (!parcel.IsContinueRead()) {
        return E_OK;
    }
    std::vector<uint64_t> dstBuf;
    parcel.ReadVector<uint64_t>(dstBuf);
    if (parcel.IsError()) {
        LOGE("[DbAbility][DeSerialize] deserialize failed.");
        return -E_LENGTH_ERROR;
    }
    if (dstBuf.size() == 0) {
        LOGE("[DbAbility][DeSerialize] buf length get failed.");
        return -E_LENGTH_ERROR;
    }
    std::vector<bool> targetBuff(SyncConfig::ABILITYBITS.back().first + SyncConfig::ABILITYBITS.back().second);
    uint32_t buffOffset = 0;
    uint32_t innerBuffOffset = 0;
    for (uint32_t pos = 0; pos < targetBuff.size() && pos < SERIALIZE_BIT_SIZE * dstBuf.size(); pos++) {
        if (innerBuffOffset >= SERIALIZE_BIT_SIZE) {
            innerBuffOffset = 0;
            buffOffset++;
        }
        targetBuff[pos] = (dstBuf[buffOffset] >> innerBuffOffset) & 0x1;
        innerBuffOffset++;
    }
    curAbility.SetDbAbilityBuff(targetBuff);
    return E_OK;
}

uint32_t DbAbility::CalculateLen(const DbAbility &curAbility)
{
    uint32_t div = curAbility.GetAbilityBitsLen() / SERIALIZE_BIT_SIZE;
    uint32_t buffLen = (curAbility.GetAbilityBitsLen() % SERIALIZE_BIT_SIZE) ? div + 1 : div;
    return Parcel::GetVectorLen<uint64_t>(std::vector<uint64_t>(buffLen, 0));
}

void DbAbility::SetDbAbilityBuff(std::vector<bool> &buff)
{
    dbAbility_ = buff;
}

const std::vector<bool> &DbAbility::GetDbAbilityBuff() const
{
    return dbAbility_;
}

uint32_t DbAbility::GetAbilityBitsLen() const
{
    return dbAbility_.size();
}

uint8_t DbAbility::GetAbilityItem(const AbilityItem &abilityType) const
{
    uint8_t data = 0;
    auto iter = dbAbilityItemSet_.find(abilityType);
    if (iter != dbAbilityItemSet_.end()) {
        if ((iter->first + iter->second) > dbAbility_.size()) {
            LOGE("[DbAbility] abilityType is error, start=%d, use_bit=%d, totalLen=%d", iter->first, iter->second,
                dbAbility_.size());
            return 0;
        }
        uint32_t skip = 0;
        // dbAbility_ bit[0..len] : low-->high, skip range 0..7
        for (uint32_t pos = iter->first; pos < (iter->first + iter->second); pos++, skip++) {
            if (dbAbility_[pos]) {
                data += (static_cast<uint8_t>(dbAbility_[pos])) << skip;
            }
        }
    }
    return data;
}

int DbAbility::SetAbilityItem(const AbilityItem &abilityType, uint8_t data)
{
    auto iter = dbAbilityItemSet_.find(abilityType);
    if (iter != dbAbilityItemSet_.end()) {
        if (data >= pow(2, iter->second)) { // 2: means binary
            LOGE("[DbAbility] value is invalid, data=%d, use_bit=%d", data, iter->second);
            return -E_INTERNAL_ERROR;
        }
        if ((iter->first + iter->second) > dbAbility_.size()) {
            dbAbility_.resize(iter->first + iter->second);
        }
        int pos = iter->first;
        while (data) {
            dbAbility_[pos] = data % 2; // 2: means binary
            data = (data >> 1);
            pos++;
        }
    }
    return E_OK;
}
} // namespace DistributedDB
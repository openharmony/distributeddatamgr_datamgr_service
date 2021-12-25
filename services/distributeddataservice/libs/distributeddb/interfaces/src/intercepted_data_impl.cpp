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

#include "intercepted_data_impl.h"
#include "db_common.h"
#include "db_constant.h"
#include "generic_single_ver_kv_entry.h"
#include "parcel.h"
#include "version.h"

namespace DistributedDB {
namespace {
bool CheckKey(const Key &key)
{
    if (key.empty() || key.size() > DBConstant::MAX_KEY_SIZE) {
        LOGE("Key is too large:%zu.", key.size());
        return false;
    }
    return true;
}

bool CheckValue(const Value &value, const std::function<int(const Value &)> &checkSchema)
{
    if (value.size() > DBConstant::MAX_VALUE_SIZE) {
        LOGE("Value is too large:%zu.", value.size());
        return false;
    }

    if (checkSchema == nullptr) {
        LOGE("Check schema failed, no check func.");
        return false;
    }

    int errCode = checkSchema(value);
    if (errCode != E_OK) {
        LOGE("Check schema failed, value is invalid:%d.", errCode);
        return false;
    }
    return true;
}

bool CheckLength(size_t len, size_t maxPacketSize)
{
    if (len > maxPacketSize) {
        LOGE("Packet is too large:%zu.", len);
        return false;
    }
    return true;
}
} // anonymous namespace

InterceptedDataImpl::InterceptedDataImpl(std::vector<SingleVerKvEntry *> dataItems,
    const std::function<int(const Value&)> &checkSchema)
    : kvEntriesReady_(false),
      isError_(false),
      totalLength_(),
      maxPacketSize_(),
      checkSchema_(checkSchema),
      dataItems_(dataItems),
      kvEntries_(),
      indexes_()
{
    totalLength_ = GenericSingleVerKvEntry::CalculateLens(dataItems, SOFTWARE_VERSION_CURRENT);
    // New packet cannot exceed both twice the MTU and twice the original size.
    // Besides, it cannot exceed 30 MB.
    maxPacketSize_ = std::min(DBConstant::MAX_SYNC_BLOCK_SIZE,
                              std::max(totalLength_, static_cast<size_t>(DBConstant::MAX_MTU_SIZE)) * 2);
}

InterceptedDataImpl::~InterceptedDataImpl()
{}

std::vector<KVEntry> InterceptedDataImpl::GetEntries()
{
    if (!kvEntriesReady_) {
        GetKvEntries();
    }
    return kvEntries_;
}

bool InterceptedDataImpl::CheckIndex(size_t index)
{
    if (!kvEntriesReady_) {
        GetKvEntries();
    }

    if (index >= kvEntries_.size()) {
        LOGE("Index is too large:%zu, size:%zu.", index, kvEntries_.size());
        return false;
    }
    return true;
}

DBStatus InterceptedDataImpl::ModifyKey(size_t index, const Key &newKey)
{
    // Check index.
    if (!CheckIndex(index)) {
        isError_ = true;
        return INVALID_ARGS;
    }

    // Check key.
    if (!CheckKey(newKey)) {
        isError_ = true;
        return INVALID_ARGS;
    }

    // Check length.
    const auto &oldKey = dataItems_[indexes_[index]]->GetKey();
    size_t newLength = totalLength_ - Parcel::GetVectorCharLen(oldKey) + Parcel::GetVectorCharLen(newKey);
    if (!CheckLength(newLength, maxPacketSize_)) {
        isError_ = true;
        return INVALID_ARGS;
    }
    totalLength_ = newLength;

    // Modify data
    auto entry = dataItems_[indexes_[index]];
    entry->SetKey(newKey);
    Key hashKey;
    int errCode = DBCommon::CalcValueHash(newKey, hashKey);
    if (errCode != E_OK) {
        LOGE("Calc hashkey failed.");
        isError_ = true;
        return INVALID_ARGS;
    }
    entry->SetHashKey(hashKey);
    return OK;
}

DBStatus InterceptedDataImpl::ModifyValue(size_t index, const Value &newValue)
{
    // Check index.
    if (!CheckIndex(index)) {
        isError_ = true;
        return INVALID_ARGS;
    }

    // Check value.
    if (!CheckValue(newValue, checkSchema_)) {
        isError_ = true;
        return INVALID_ARGS;
    }

    // Check length.
    const auto &oldValue = dataItems_[indexes_[index]]->GetValue();
    size_t newLength = totalLength_ - Parcel::GetVectorCharLen(oldValue) + Parcel::GetVectorCharLen(newValue);
    if (!CheckLength(newLength, maxPacketSize_)) {
        isError_ = true;
        return INVALID_ARGS;
    }
    totalLength_ = newLength;

    // Modify data
    auto entry = dataItems_[indexes_[index]];
    entry->SetValue(newValue);
    return OK;
}

bool InterceptedDataImpl::IsError() const
{
    return isError_;
}

void InterceptedDataImpl::GetKvEntries()
{
    for (size_t i = 0; i < dataItems_.size(); ++i) {
        const auto &kvEntry = dataItems_[i];
        if ((kvEntry->GetFlag() & DataItem::DELETE_FLAG) == 0) { // For deleted data, do not modify.
            kvEntries_.push_back({ kvEntry->GetKey(), kvEntry->GetValue() });
            indexes_.push_back(i);
        }
    }
    kvEntriesReady_ = true;
}
} // namespace DistributedDB


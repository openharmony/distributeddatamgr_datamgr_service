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

#include "typesutil_fuzzer.h"

#include <cstdint>
#include <variant>
#include <vector>

#include "itypes_util.h"
#include "types.h"

using namespace OHOS::DistributedKv;
namespace OHOS {
void clientDevFuzz(const std::string &stringBase)
{
    MessageParcel parcel;
    DeviceInfo clientDev;
    clientDev.deviceId = stringBase;
    clientDev.deviceName = stringBase;
    clientDev.deviceType = stringBase;
    ITypesUtil::Marshal(parcel, clientDev);
    DeviceInfo serverDev;
    ITypesUtil::Unmarshal(parcel, serverDev);
}

void EntryFuzz(const std::string &stringBase)
{
    MessageParcel parcel;
    Entry entryIn;
    entryIn.key = stringBase;
    entryIn.value = stringBase;
    ITypesUtil::Marshal(parcel, entryIn);
    Entry entryOut;
    ITypesUtil::Unmarshal(parcel, entryOut);
}

void BlobFuzz(const std::string &stringBase)
{
    Blob blobIn = stringBase;
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, blobIn);
    Blob blobOut;
    ITypesUtil::Unmarshal(parcel, blobOut);
}

void VecFuzz(const std::vector<uint8_t> &vec)
{
    std::vector<uint8_t> vecIn(vec);
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, vecIn);
    std::vector<uint8_t> vecOut;
    ITypesUtil::Unmarshal(parcel, vecOut);
}

void OptionsFuzz(const std::string &stringBase)
{
    Options optionsIn = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION };
    optionsIn.area = EL1;
    optionsIn.baseDir = stringBase;
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, optionsIn);
    Options optionsOut;
    ITypesUtil::Unmarshal(parcel, optionsOut);
}

void SyncPolicyFuzz(uint32_t base)
{
    SyncPolicy syncPolicyIn { base, base };
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, syncPolicyIn);
    SyncPolicy syncPolicyOut;
    ITypesUtil::Unmarshal(parcel, syncPolicyOut);
}

void ChangeNotificationFuzz(const std::string &stringBase, bool boolBase)
{
    Entry insert, update, del;
    insert.key = "insert_" + stringBase;
    update.key = "update_" + stringBase;
    del.key = "delete_" + stringBase;
    insert.value = "insert_value_" + stringBase;
    update.value = "update_value_" + stringBase;
    del.value = "delete_value_" + stringBase;
    std::vector<Entry> inserts, updates, deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), stringBase, boolBase);
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, changeIn);
    std::vector<Entry> empty;
    ChangeNotification changeOut(std::move(empty), {}, {}, "", !boolBase);
    ITypesUtil::Unmarshal(parcel, changeOut);
}

void IntFuzz(size_t valBase)
{
    MessageParcel parcel;
    int32_t int32In = static_cast<int32_t>(valBase);
    ITypesUtil::Marshal(parcel, int32In);
    int32_t int32Out;
    ITypesUtil::Unmarshal(parcel, int32Out);

    uint32_t uint32In = static_cast<uint32_t>(valBase);
    ITypesUtil::Marshal(parcel, uint32In);
    uint32_t uint32Out;
    ITypesUtil::Unmarshal(parcel, uint32Out);

    uint64_t uint64In = static_cast<uint64_t>(valBase);
    ITypesUtil::Marshal(parcel, uint64In);
    uint64_t uint64Out;
    ITypesUtil::Unmarshal(parcel, uint64Out);
}

void StringFuzz(const std::string &stringBase)
{
    MessageParcel parcel;
    std::string stringIn = stringBase;
    ITypesUtil::Marshal(parcel, stringIn);
    std::string stringOut;
    ITypesUtil::Unmarshal(parcel, stringOut);
}

void RdbSyncerParamFuzz(const std::string &stringBase, int32_t intBase, const std::vector<uint8_t> &vecBase, bool boolBase)
{
    MessageParcel parcel;
    DistributedRdb::RdbSyncerParam rdbSyncerParamIn;
    rdbSyncerParamIn.bundleName_ = stringBase;
    rdbSyncerParamIn.hapName_ = stringBase;
    rdbSyncerParamIn.storeName_ = stringBase;
    rdbSyncerParamIn.area_ = intBase;
    rdbSyncerParamIn.level_ = intBase;
    rdbSyncerParamIn.type_ = intBase;
    rdbSyncerParamIn.isAutoSync_ = boolBase;
    rdbSyncerParamIn.isEncrypt_ = boolBase;
    rdbSyncerParamIn.password_ = vecBase;
    ITypesUtil::Marshal(parcel, rdbSyncerParamIn);
    DistributedRdb::RdbSyncerParam rdbSyncerParamOut;
    ITypesUtil::Unmarshal(parcel, rdbSyncerParamOut);
}

void RdbSyncOptionFuzz(bool boolBase)
{
    MessageParcel parcel;
    DistributedRdb::SyncOption syncOptionIn = { DistributedRdb::PUSH, boolBase };
    ITypesUtil::Marshal(parcel, syncOptionIn);
    DistributedRdb::SyncOption syncOptionOut;
    ITypesUtil::Unmarshal(parcel, syncOptionOut);
}

void RdbPredicatesFuzz(const std::string &stringBase)
{
    MessageParcel parcel;
    DistributedRdb::RdbPredicates rdbPredicatesIn;
    rdbPredicatesIn.table_ = stringBase;
    rdbPredicatesIn.devices_ = { stringBase };
    rdbPredicatesIn.operations_ = { { DistributedRdb::EQUAL_TO, stringBase, { stringBase } } };
    ITypesUtil::Marshal(parcel, rdbPredicatesIn);
    DistributedRdb::RdbPredicates rdbPredicatesOut;
    ITypesUtil::Unmarshal(parcel, rdbPredicatesOut);
}

void GetTotalSizeFuzz(const std::string &stringBase, uint32_t size)
{
    std::vector<Entry> VecEntryIn(size);
    std::vector<Key> VecKeyIn(size);
    for (auto entry : VecEntryIn) {
        entry.value = stringBase;
        entry.key = stringBase;
    }
    ITypesUtil::GetTotalSize(VecEntryIn);

    for (auto key : VecKeyIn) {
        key = stringBase;
    }
    ITypesUtil::GetTotalSize(VecKeyIn);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    bool fuzzedBoolean = ((size % 2) == 0);
    int32_t fuzzedInt32 = static_cast<int32_t>(size);
    uint32_t fuzzedUInt32 = static_cast<uint32_t>(size);
    std::string fuzzedString(reinterpret_cast<const char *>(data), size);
    std::vector<uint8_t> fuzzedVec(fuzzedString.begin(), fuzzedString.end());

    OHOS::clientDevFuzz(fuzzedString);
    OHOS::EntryFuzz(fuzzedString);
    OHOS::BlobFuzz(fuzzedString);
    OHOS::VecFuzz(fuzzedVec);
    OHOS::OptionsFuzz(fuzzedString);
    OHOS::SyncPolicyFuzz(fuzzedUInt32);
    OHOS::ChangeNotificationFuzz(fuzzedString, fuzzedBoolean);
    OHOS::IntFuzz(size);
    OHOS::StringFuzz(fuzzedString);
    OHOS::RdbSyncerParamFuzz(fuzzedString, fuzzedInt32, fuzzedVec, fuzzedBoolean);
    OHOS::RdbSyncOptionFuzz(fuzzedBoolean);
    OHOS::RdbPredicatesFuzz(fuzzedString);
    OHOS::GetTotalSizeFuzz(fuzzedString, fuzzedUInt32);
    return 0;
}

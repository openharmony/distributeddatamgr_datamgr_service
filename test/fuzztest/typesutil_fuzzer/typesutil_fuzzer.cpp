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
void ClientDevFuzz(const std::string &strBase)
{
    DeviceInfo clientDev;
    clientDev.deviceId = strBase;
    clientDev.deviceName = strBase;
    clientDev.deviceType = strBase;
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, clientDev);
    DeviceInfo serverDev;
    ITypesUtil::Unmarshal(parcel, serverDev);
}

void EntryFuzz(const std::string &strBase)
{
    Entry entryIn;
    entryIn.key = strBase;
    entryIn.value = strBase;
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, entryIn);
    Entry entryOut;
    ITypesUtil::Unmarshal(parcel, entryOut);
}

void BlobFuzz(const std::string &strBase)
{
    Blob blobIn = strBase;
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

void OptionsFuzz(const std::string &strBase)
{
    Options optionsIn = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION
    };
    optionsIn.area = EL1;
    optionsIn.baseDir = strBase;
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

void ChangeNotificationFuzz(const std::string &strBase, bool boolBase)
{
    Entry insert, update, del;
    insert.key = strBase;
    update.key = strBase;
    del.key = strBase;
    insert.value = strBase;
    update.value = strBase;
    del.value = strBase;
    std::vector<Entry> inserts, updates, deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), strBase, boolBase);
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

void StringFuzz(const std::string &strBase)
{
    MessageParcel parcel;
    std::string strIn = strBase;
    ITypesUtil::Marshal(parcel, strIn);
    std::string strOut;
    ITypesUtil::Unmarshal(parcel, strOut);
}

void RdbSyncerParamFuzz(const std::string &strBase, int32_t intBase, const std::vector<uint8_t> &vecBase,
    bool boolBase)
{
    MessageParcel parcel;
    DistributedRdb::RdbSyncerParam rdbSyncerParamIn;
    rdbSyncerParamIn.bundleName_ = strBase;
    rdbSyncerParamIn.hapName_ = strBase;
    rdbSyncerParamIn.storeName_ = strBase;
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

void RdbPredicatesFuzz(const std::string &strBase)
{
    MessageParcel parcel;
    DistributedRdb::RdbPredicates rdbPredicatesIn;
    rdbPredicatesIn.table_ = strBase;
    rdbPredicatesIn.devices_ = { strBase };
    rdbPredicatesIn.operations_ = { { DistributedRdb::EQUAL_TO, strBase, { strBase } } };
    ITypesUtil::Marshal(parcel, rdbPredicatesIn);
    DistributedRdb::RdbPredicates rdbPredicatesOut;
    ITypesUtil::Unmarshal(parcel, rdbPredicatesOut);
}

void GetTotalSizeFuzz(const std::string &strBase, uint32_t size)
{
    Entry entry;
    entry.key = strBase;
    entry.value = strBase;
    std::vector<Entry> VecEntryIn(size, entry);
    std::vector<Key> VecKeyIn(size, Key { strBase });
    ITypesUtil::GetTotalSize(VecEntryIn);
    ITypesUtil::GetTotalSize(VecKeyIn);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    bool fuzzBool = ((size % 2) == 0);
    int32_t fuzzInt32 = static_cast<int32_t>(size);
    uint32_t fuzzUInt32 = static_cast<uint32_t>(size);
    std::string fuzzStr(reinterpret_cast<const char *>(data), size);
    std::vector<uint8_t> fuzzVec(fuzzStr.begin(), fuzzStr.end());

    OHOS::ClientDevFuzz(fuzzStr);
    OHOS::EntryFuzz(fuzzStr);
    OHOS::BlobFuzz(fuzzStr);
    OHOS::VecFuzz(fuzzVec);
    OHOS::OptionsFuzz(fuzzStr);
    OHOS::SyncPolicyFuzz(fuzzUInt32);
    OHOS::ChangeNotificationFuzz(fuzzStr, fuzzBool);
    OHOS::IntFuzz(size);
    OHOS::StringFuzz(fuzzStr);
    OHOS::RdbSyncerParamFuzz(fuzzStr, fuzzInt32, fuzzVec, fuzzBool);
    OHOS::RdbSyncOptionFuzz(fuzzBool);
    OHOS::RdbPredicatesFuzz(fuzzStr);
    OHOS::GetTotalSizeFuzz(fuzzStr, fuzzUInt32);
    return 0;
}

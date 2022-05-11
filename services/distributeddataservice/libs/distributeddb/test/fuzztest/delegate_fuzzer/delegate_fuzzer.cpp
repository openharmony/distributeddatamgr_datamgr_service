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

#include "delegate_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
    std::vector<Entry> CreateEntries(const uint8_t* data, size_t size, std::vector<Key>& keys)
    {
        std::vector<Entry> entries;
        // key'length is less than 1024.
        auto count = static_cast<int>(std::min(size, size_t(1024)));
        for (int i = 1; i < count; i++) {
            Entry entry;
            entry.key = std::vector<uint8_t>(data, data + 1);
            entry.value = std::vector<uint8_t>(data, data + size);
            keys.push_back(entry.key);
            entries.push_back(entry);
        }
        return entries;
    }

    void MultiCombineFuzzer(const uint8_t* data, size_t size, KvStoreDelegate::Option &option)
    {
        static auto kvManger = KvStoreDelegateManager("APP_ID", "USER_ID");
        KvStoreConfig config;
        DistributedDBToolsTest::TestDirInit(config.dataDir);
        kvManger.SetKvStoreConfig(config);
        KvStoreDelegate *kvDelegatePtr = nullptr;
        kvManger.GetKvStore("distributed_delegate_test", option,
            [&kvDelegatePtr](DBStatus status, KvStoreDelegate* kvDelegate) {
                if (status == DBStatus::OK) {
                    kvDelegatePtr = kvDelegate;
                }
            });
        KvStoreObserverTest *observer = new (std::nothrow) KvStoreObserverTest;
        if ((kvDelegatePtr == nullptr) || (observer == nullptr)) {
            return;
        }

        kvDelegatePtr->RegisterObserver(observer);
        Key key = std::vector<uint8_t>(data, data + (size % 1024)); /* 1024 is max */
        Value value = std::vector<uint8_t>(data, data + size);
        kvDelegatePtr->Put(key, value);
        KvStoreSnapshotDelegate* kvStoreSnapshotPtr = nullptr;
        kvDelegatePtr->GetKvStoreSnapshot(nullptr,
            [&kvStoreSnapshotPtr](DBStatus status, KvStoreSnapshotDelegate* kvStoreSnapshot) {
                kvStoreSnapshotPtr = std::move(kvStoreSnapshot);
            });
        if (kvStoreSnapshotPtr == nullptr) {
            return;
        }
        auto valueCallback = [&value] (DBStatus status, const Value &getValue) {
            value = getValue;
        };

        kvStoreSnapshotPtr->Get(key, valueCallback);
        kvDelegatePtr->Delete(key);
        kvStoreSnapshotPtr->Get(key, valueCallback);
        std::vector<Key> keys;
        kvDelegatePtr->PutBatch(CreateEntries(data, size, keys));
        Key keyPrefix = std::vector<uint8_t>(data, data + 1);
        kvStoreSnapshotPtr->GetEntries(keyPrefix, [](DBStatus status, const std::vector<Entry> &entries) {
            (void) entries.size();
        });

        kvDelegatePtr->DeleteBatch(keys);
        kvDelegatePtr->Clear();
        kvDelegatePtr->UnRegisterObserver(observer);
        kvDelegatePtr->ReleaseKvStoreSnapshot(kvStoreSnapshotPtr);
        kvManger.CloseKvStore(kvDelegatePtr);
        kvManger.DeleteKvStore("distributed_delegate_test");
        DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    CipherPassword passwd;
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(data, size, option);
    option = {true, false, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(data, size, option);
    return 0;
}


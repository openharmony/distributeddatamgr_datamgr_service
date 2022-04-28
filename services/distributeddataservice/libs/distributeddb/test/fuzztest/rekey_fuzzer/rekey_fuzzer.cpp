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

#include "rekey_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
    static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    std::vector<Entry> CreateEntries(const uint8_t* data, size_t size)
    {
        std::vector<Entry> entries;

        auto count = static_cast<int>(std::min(size, size_t(1024)));
        for(int i = 1; i < count; i++) {
            Entry entry;
            entry.key = std::vector<uint8_t> (data, data + i);
            entry.value = std::vector<uint8_t> (data, data + size);
            entries.push_back(entry);
        }
        return entries;
    }

    void SingerVerReKey(const uint8_t* data, size_t size)
    {
        CipherPassword passwd;
        passwd.SetValue(data, (size / 2));

        KvStoreNbDelegate::Option nbOption = {true, false, true, CipherType::DEFAULT, passwd};
        KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

        g_kvManager.GetKvStore("distributed_nb_rekey_test", nbOption,
            [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
                if (status == DBStatus::OK) {
                    kvNbDelegatePtr = kvNbDelegate;
                }
            });

        if (kvNbDelegatePtr) {
            kvNbDelegatePtr->PutBatch(CreateEntries(data, size));
            passwd.SetValue(data, size);
            kvNbDelegatePtr->Rekey(passwd);
            g_kvManager.CloseKvStore(kvNbDelegatePtr);
        }
    }

    void MultiVerVerReKey(const uint8_t* data, size_t size)
    {
        CipherPassword passwd;
        passwd.SetValue(data, (size / 2));

        KvStoreNbDelegate::Option nbOption = {true, false, true, CipherType::DEFAULT, passwd};
        KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

        g_kvManager.GetKvStore("distributed_rekey_test", nbOption,
            [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
                if (status == DBStatus::OK) {
                    kvNbDelegatePtr = kvNbDelegate;
                }
            });

        if (kvNbDelegatePtr) {
            kvNbDelegatePtr->PutBatch(CreateEntries(data, size));
            CipherPassword passwd;
            passwd.SetValue(data, size);
            kvNbDelegatePtr->Rekey(passwd);
            g_kvManager.CloseKvStore(kvNbDelegatePtr);
        }
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    OHOS::SingerVerReKey(data, size);
    OHOS::MultiVerVerReKey(data, size);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}


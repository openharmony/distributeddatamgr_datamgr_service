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

#include "kvstoreresultset_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "process_communicator_test_stub.h"

using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace std;

namespace OHOS {
static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
void ResultSetFuzzer(const uint8_t* data, size_t size)
{
    KvStoreNbDelegate::Option option = {true, false, true};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_nb_delegate_result_set_test", option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return;
    }

    Key testKey;
    Value testValue;
    for (size_t i = 0; i < size; i++) {
        testKey.clear();
        testValue.clear();
        testKey.push_back(data[i]);
        testValue.push_back(data[i]);
        kvNbDelegatePtr->Put(testKey, testValue);
    }

    Key keyPrefix;
    KvStoreResultSet *readResultSet = nullptr;
    kvNbDelegatePtr->GetEntries(keyPrefix, readResultSet);
    if (readResultSet != nullptr) {
        readResultSet->GetCount();
        readResultSet->GetPosition();
        readResultSet->MoveToNext();
        readResultSet->MoveToPrevious();
        readResultSet->MoveToFirst();
        readResultSet->MoveToLast();

        if (size == 0) {
            return;
        }
        auto pos = U32_AT(data) % size;
        readResultSet->MoveToPosition(pos++);
        readResultSet->Move(0 - pos);
        readResultSet->IsFirst();
        readResultSet->IsLast();
        readResultSet->IsBeforeFirst();
        readResultSet->IsAfterLast();
        kvNbDelegatePtr->CloseResultSet(readResultSet);
    }

    g_kvManager.CloseKvStore(kvNbDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_nb_delegate_result_set_test");
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // u32 4 bytes
    if (size < 4) {
        return 0;
    }
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    OHOS::ResultSetFuzzer(data, size);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}


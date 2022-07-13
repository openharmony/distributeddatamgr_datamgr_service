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

#include "kvstoredisksize_fuzzer.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;
namespace OHOS {
static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
void NbDbTest(const uint8_t *data, size_t size)
{
    std::string rawString(reinterpret_cast<const char *>(data), size);
    CipherPassword passwd;
    passwd.SetValue(data, size);
    KvStoreNbDelegate::Option nbOption {true, true, false, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    g_kvManager.GetKvStore(rawString, nbOption,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
    });

    if (kvNbDelegatePtr != nullptr) {
        Key key;
        Value value;
        DistributedDBToolsTest::GetRandomKeyValue(key, DBConstant::MAX_KEY_SIZE);
        DistributedDBToolsTest::GetRandomKeyValue(value, DBConstant::MAX_VALUE_SIZE);
        kvNbDelegatePtr->Put(key, value);
        g_kvManager.CloseKvStore(kvNbDelegatePtr);
    }
    uint64_t dbSize = 0;
    g_kvManager.GetKvStoreDiskSize(rawString, dbSize);
    g_kvManager.DeleteKvStore(rawString);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    std::string rawString(reinterpret_cast<const char *>(data), size);
    KvStoreDelegateManager::SetProcessLabel(rawString, rawString);
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    OHOS::NbDbTest(data, size);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}


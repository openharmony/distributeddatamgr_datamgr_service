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

#include "autolaunch_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace DistributedDBUnitTest;
namespace OHOS {
static auto g_kvManager = KvStoreDelegateManager(APP_ID, USER_ID);

void EnableAutoLaunchFuzz(const uint8_t *data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_kvManager.SetKvStoreConfig(config);
    std::string rawString(reinterpret_cast<const char *>(data), size);
    CipherPassword passwd;
    passwd.SetValue(data, size);
    AutoLaunchOption launchOption;
    KvStoreDelegateManager::EnableKvStoreAutoLaunch(USER_ID, APP_ID, rawString, launchOption, nullptr);
    AutoLaunchOption launchOption1 {true, false, CipherType::DEFAULT, passwd, "", false, config.dataDir, nullptr};
    KvStoreDelegateManager::EnableKvStoreAutoLaunch(USER_ID, APP_ID, "StoreId1", launchOption1, nullptr);
    AutoLaunchOption launchOption2 {true, true, CipherType::DEFAULT, passwd, "", false, config.dataDir, nullptr};
    KvStoreDelegateManager::EnableKvStoreAutoLaunch(USER_ID, APP_ID, "StoreId2", launchOption2, nullptr);
    AutoLaunchOption launchOption3 {false, true, CipherType::DEFAULT, passwd, "", false, config.dataDir, nullptr};
    KvStoreDelegateManager::EnableKvStoreAutoLaunch(USER_ID, APP_ID, "StoreId3", launchOption3, nullptr);
    AutoLaunchOption launchOption4 {false, false, CipherType::DEFAULT, passwd, "", false, config.dataDir, nullptr};
    KvStoreDelegateManager::EnableKvStoreAutoLaunch(USER_ID, APP_ID, "StoreId4", launchOption4, nullptr);
}

void DisableAutoLaunchFUzz(const uint8_t *data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_kvManager.SetKvStoreConfig(config);
    std::string rawString(reinterpret_cast<const char *>(data), size);
    CipherPassword passwd;
    passwd.SetValue(data, size);
    KvStoreDelegateManager::DisableKvStoreAutoLaunch(USER_ID, APP_ID, rawString);
    passwd.SetValue(data, size);
    KvStoreNbDelegate::Option option {true, true, false, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    g_kvManager.GetKvStore(rawString, option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
    });
    KvStoreDelegateManager::DisableKvStoreAutoLaunch(USER_ID, APP_ID, rawString);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::EnableAutoLaunchFuzz(data, size);
    OHOS::DisableAutoLaunchFUzz(data, size);
    return 0;
}


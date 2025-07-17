/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObjectManagerTest"

#include "object_manager.h"

#include <gtest/gtest.h>
#include <ipc_skeleton.h>

#include "bootstrap.h"
#include "device_manager_adapter_mock.h"
#include "executor_pool.h"
#include "kvstore_meta_manager.h"
#include "kv_store_nb_delegate_mock.h"
#include "object_types.h"
#include "snapshot/machine_status.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace std;
using namespace testing;
using AssetValue = OHOS::CommonType::AssetValue;
using RestoreStatus = OHOS::DistributedObject::ObjectStoreManager::RestoreStatus;
namespace OHOS::Test {

class ObjectManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

protected:
    Asset asset_;
    std::string uri_;
    std::string appId_ = "objectManagerTest_appid_1";
    std::string sessionId_ = "123";
    std::vector<uint8_t> data_;
    std::string deviceId_ = "7001005458323933328a258f413b3900";
    uint64_t sequenceId_ = 10;
    uint64_t sequenceId_2 = 20;
    uint64_t sequenceId_3 = 30;
    std::string userId_ = "100";
    std::string bundleName_ = "com.examples.hmos.notepad";
    OHOS::ObjectStore::AssetBindInfo assetBindInfo_;
    pid_t pid_ = 10;
    uint32_t tokenId_ = 100;
    AssetValue assetValue_;
    static inline std::shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
};
} // namespace OHOS::Test

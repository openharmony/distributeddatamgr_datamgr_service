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

#include "auth_delegate.h"
#include "bootstrap.h"
#include "executor_pool.h"
#include <memory>
#include "metadata/store_meta_data.h"
#include <nlohmann/json.hpp>
#include "kvstore_client_death_observer.h"
#include "bootstrap.h"
#include "gtest/gtest.h"
#include "kvstore_data_service.h"
#include "serializable/serializable.h"
#include "system_ability.h"
#include "system_ability_definition.h"
#include "upgrade_manager.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS;
using StoreMetaData = DistributedData::StoreMetaData;
namespace OHOS::Test {
class KvStoreDataServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static constexpr int32_t TEST_USERID = 100;
    static constexpr int32_t TEST_APP_INDEX = 0;
    static constexpr int32_t TEST_TOKENID = 100;
    std::string TEST_INDENTATION = "    ";
};

void KvStoreDataServiceTest::SetUpTestCase(void)
{}

void KvStoreDataServiceTest::TearDownTestCase(void)
{}

void KvStoreDataServiceTest::SetUp(void)
{}

void KvStoreDataServiceTest::TearDown(void)
{}

class UpgradeManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void UpgradeManagerTest::SetUpTestCase(void)
{}

void UpgradeManagerTest::TearDownTestCase(void)
{}

void UpgradeManagerTest::SetUp(void)
{}

void UpgradeManagerTest::TearDown(void)
{}

/**
* @tc.name: RegisterClientDeathObserver001
* @tc.desc: register client death observer
* @tc.type: FUNC
* @tc.require: AR000CQDU2
* @tc.author: liuyuhui
*/
HWTEST_F(KvStoreDataServiceTest, RegisterClientDeathObserver001, TestSize.Level1)
{
    AppId appId;
    appId.appId = "app0";
    KvStoreDataService kvDataService;
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadCheckers();
    KvStoreMetaManager::GetInstance().BindExecutor(std::make_shared<ExecutorPool>(12, 5));
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    Status status = kvDataService.RegisterClientDeathObserver(appId, new KvStoreClientDeathObserver());
    EXPECT_EQ(status, Status::SUCCESS) << "RegisterClientDeathObserver failed";
}

/**
* @tc.name: GetIndentation001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, GetIndentation001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string indentation = kvStoreDataServiceTest.GetIndentation(1);
    EXPECT_EQ(indentation, TEST_INDENTATION);
}

/**
* @tc.name: GetFeatureInterface001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, GetFeatureInterface001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string name = "kvstoredatatest";
    sptr<IRemoteObject> object = kvStoreDataServiceTest.GetFeatureInterface(name);
    EXPECT_EQ(object, nullptr);
}

/**
* @tc.name: Dump001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, Dump001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::vector<std::u16string> args;
    const std::u16string argstest1 = u"OHOS.DistributedKv.IKvStoreDataService1";
    const std::u16string argstest2 = u"OHOS.DistributedKv.IKvStoreDataService2";
    args.emplace_back(argstest1);
    args.emplace_back(argstest2);
    kvStoreDataServiceTest.OnDump();
    int32_t status = kvStoreDataServiceTest.Dump(1, args);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: OnUninstall001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, OnUninstall001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string bundleNametest = "ohos.kvstoredatatest.demo";
    int32_t status = kvStoreDataServiceTest.OnUninstall(bundleNametest, TEST_USERID, TEST_APP_INDEX);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: OnUpdate001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, OnUpdate001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string bundleNametest = "ohos.kvstoredatatest.demo";
    int32_t status = kvStoreDataServiceTest.OnUpdate(bundleNametest, TEST_USERID, TEST_APP_INDEX);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: OnInstall001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, OnInstall001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string bundleNametest = "ohos.kvstoredatatest.demo";
    int32_t status = kvStoreDataServiceTest.OnInstall(bundleNametest, TEST_USERID, TEST_APP_INDEX);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: AppExit001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, AppExit001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    pid_t uid = 1;
    pid_t pid = 2;
    uint32_t token = 3;
    OHOS::DistributedKv::AppId appId = { "ohos.test.kvstoredataservice" };
    Status status = kvStoreDataServiceTest.AppExit(uid, pid, token, appId);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: OnRemoveSystemAbility001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnRemoveSystemAbility001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    int32_t systemAbilityId = MEMORY_MANAGER_SA_ID;
    std::string deviceId = "ohos.test.kvstoredataservice";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnRemoveSystemAbility(systemAbilityId, deviceId));

    systemAbilityId = COMMON_EVENT_SERVICE_ID;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnRemoveSystemAbility(systemAbilityId, deviceId));
}

/**
* @tc.name: OnStoreMetaChanged001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnStoreMetaChanged001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::vector<uint8_t> key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    std::vector<uint8_t> value = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHANGE_FLAG flag = CHANGE_FLAG::UPDATE;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnStoreMetaChanged(key, value, flag));
}

/**
* @tc.name: AccountEventChanged001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, AccountEventChanged001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    AccountEventInfo eventInfo;
    eventInfo.status = AccountStatus::DEVICE_ACCOUNT_DELETE;
    eventInfo.userId = "100";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.AccountEventChanged(eventInfo));

    eventInfo.userId = "";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.AccountEventChanged(eventInfo));

    eventInfo.status = AccountStatus::DEVICE_ACCOUNT_SWITCHED;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.AccountEventChanged(eventInfo));

    eventInfo.status = AccountStatus::DEVICE_ACCOUNT_UNLOCKED;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.AccountEventChanged(eventInfo));
}

/**
* @tc.name: InitSecurityAdapter001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, InitSecurityAdapter001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::shared_ptr<ExecutorPool> executors2 = std::make_shared<ExecutorPool>(1, 0);
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.InitSecurityAdapter(executors2));
}

/**
* @tc.name: OnDeviceOnline001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOnline001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    AppDistributedKv::DeviceInfo info1;
    info1.uuid = "";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOnline(info1));

    AppDistributedKv::DeviceInfo info;
    info.uuid = "ohos.test.uuid";
    info.udid = "ohos.test.udid";
    info.networkId = "ohos.test.networkId";
    info.deviceName = "ohos.test.deviceName";
    info.deviceType = 1;
    info.osType = 1;
    info.authForm = 1;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOnline(info));
}

/**
* @tc.name: OnDeviceOffline001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOffline001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    AppDistributedKv::DeviceInfo info1;
    info1.uuid = "";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOffline(info1));

    AppDistributedKv::DeviceInfo info;
    info.uuid = "ohos.test.uuid";
    info.udid = "ohos.test.udid";
    info.networkId = "ohos.test.networkId";
    info.deviceName = "ohos.test.deviceName";
    info.deviceType = 1;
    info.osType = 1;
    info.authForm = 1;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOffline(info));
}

/**
* @tc.name: OnDeviceOnReady001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOnReady001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    AppDistributedKv::DeviceInfo info1;
    info1.uuid = "";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOnReady(info1));

    AppDistributedKv::DeviceInfo info;
    info.uuid = "ohos.test.uuid";
    info.udid = "ohos.test.udid";
    info.networkId = "ohos.test.networkId";
    info.deviceName = "ohos.test.deviceName";
    info.deviceType = 1;
    info.osType = 1;
    info.authForm = 1;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnDeviceOnReady(info));
}

/**
* @tc.name: OnSessionReady001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnSessionReady001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    AppDistributedKv::DeviceInfo info1;
    info1.uuid = "";
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnSessionReady(info1));

    AppDistributedKv::DeviceInfo info;
    info.uuid = "ohos.test.uuid";
    info.udid = "ohos.test.udid";
    info.networkId = "ohos.test.networkId";
    info.deviceName = "ohos.test.deviceName";
    info.deviceType = 1;
    info.osType = 1;
    info.authForm = 1;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnSessionReady(info));
}

/**
* @tc.name: DumpStoreInfo001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, DumpStoreInfo001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.DumpStoreInfo(fd, params));
}

/**
* @tc.name: UpgradeManagerTest001
* @tc.desc: test Init function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(UpgradeManagerTest, UpgradeManagerTest001, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    DistributedData::UpgradeManager instance;
    instance.Init(executors);
    EXPECT_TRUE(instance.executors_);

    instance.Init(executors);
    EXPECT_TRUE(instance.executors_);
}
} // namespace OHOS::Test
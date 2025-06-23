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
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include <memory>
#include "metadata/secret_key_meta_data.h"
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
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using StoreMetaData = DistributedData::StoreMetaData;

namespace OHOS::Test {
const std::string SECRETKEY_BACKUP_PATH = "/data/service/el1/public/database/backup_test/";
const std::string SECRETKEY_BACKUP_FILE = SECRETKEY_BACKUP_PATH + "secret_key_backup.conf";
const std::string NORMAL_CLONE_INFO =
    "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
    "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
    "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
    "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
    "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
    "16\"}},{\"type\":\"application_selection\",\"detail\":[{"
    "\"bundleName\":\"com.example.restore_test\",\"accessTokenId\":"
    "536973769}]},{\"type\":\"userId\",\"detail\":\"100\"}]";
const std::string NORMAL_BACKUP_DATA =
    "{\"infos\":[{\"bundleName\":\"com.myapp.example\",\"dbName\":"
    "\"storeId\",\"instanceId\":0,\"storeType\":\"1\",\"user\":\"100\","
    "\"sKey\":\"9aJQwx3XD3EN7To2j/I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
    "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]},{"
    "\"bundleName\":\"sa1\",\"dbName\":\"storeId1\",\"instanceId\":\"0\","
    "\"dbType\":\"1\",\"user\":\"0\",\"sKey\":\"9aJQwx3XD3EN7To2j/"
    "I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
    "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]}]}";
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
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH; // 0771
    mkdir(SECRETKEY_BACKUP_PATH.c_str(), mode);
    auto executors = std::make_shared<ExecutorPool>(12, 5);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    KvStoreMetaManager::GetInstance().BindExecutor(executors);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    KvStoreMetaManager::GetInstance().InitMetaListener();
    DeviceManagerAdapter::GetInstance().Init(executors);
}

void KvStoreDataServiceTest::TearDownTestCase(void)
{
    (void)remove(SECRETKEY_BACKUP_FILE.c_str());
    (void)rmdir(SECRETKEY_BACKUP_PATH.c_str());
}

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

static int32_t WriteContentToFile(const std::string &path, const std::string &content)
{
    FILE *fp = fopen(path.c_str(), "w");
    if (!fp) {
        return -1;
    }
    size_t ret = fwrite(content.c_str(), 1, content.length(), fp);
    if (ret != content.length()) {
        (void)fclose(fp);
        return -1;
    }
    (void)fflush(fp);
    (void)fsync(fileno(fp));
    (void)fclose(fp);
    return 0;
}

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
    Status status = kvDataService.RegisterClientDeathObserver(appId, new KvStoreClientDeathObserver(), "");
    EXPECT_EQ(status, Status::SUCCESS) << "RegisterClientDeathObserver failed";
}

/**
* @tc.name: RegisterClientDeathObserver002
* @tc.desc: register client death observer
* @tc.type: FUNC
*/
HWTEST_F(KvStoreDataServiceTest, RegisterClientDeathObserver002, TestSize.Level1)
{
    AppId appId;
    appId.appId = "app0";
    KvStoreDataService kvDataService;
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadCheckers();
    KvStoreMetaManager::GetInstance().BindExecutor(std::make_shared<ExecutorPool>(12, 5));
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    Status status = kvDataService.RegisterClientDeathObserver(appId, nullptr, "");
    EXPECT_EQ(status, Status::SUCCESS) << "RegisterClientDeathObserver failed";
    for (int i = 0; i < 17; i++) {
        auto featureName = std::to_string(i);
        status = kvDataService.RegisterClientDeathObserver(appId, new KvStoreClientDeathObserver(), featureName);
        EXPECT_EQ(status, Status::SUCCESS) << "RegisterClientDeathObserver failed";
    }
}

/**
* @tc.name: Exit001
* @tc.desc: feature Exit
* @tc.type: FUNC
*/
HWTEST_F(KvStoreDataServiceTest, Exit001, TestSize.Level1)
{
    KvStoreDataService kvDataService;
    EXPECT_EQ(kvDataService.Exit(""), Status::SUCCESS);
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
* @tc.desc: test OnRemoveSystemAbility function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnRemoveSystemAbility001, TestSize.Level0)
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
* @tc.desc: test OnStoreMetaChanged function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnStoreMetaChanged001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::vector<uint8_t> key = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    std::vector<uint8_t> value = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    CHANGE_FLAG flag = CHANGE_FLAG::UPDATE;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.OnStoreMetaChanged(key, value, flag));
}

/**
* @tc.name: AccountEventChanged001
* @tc.desc: test AccountEventChanged function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, AccountEventChanged001, TestSize.Level0)
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
* @tc.desc: test InitSecurityAdapter function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, InitSecurityAdapter001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::shared_ptr<ExecutorPool> executors2 = std::make_shared<ExecutorPool>(1, 0);
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.InitSecurityAdapter(executors2));
}

/**
* @tc.name: OnDeviceOnline001
* @tc.desc: test OnDeviceOnline function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOnline001, TestSize.Level0)
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
* @tc.desc: test OnDeviceOffline function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOffline001, TestSize.Level0)
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
* @tc.desc: test OnDeviceOnReady function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnDeviceOnReady001, TestSize.Level0)
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
* @tc.desc: test OnSessionReady function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, OnSessionReady001, TestSize.Level0)
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
* @tc.desc: test DumpStoreInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, DumpStoreInfo001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.DumpStoreInfo(fd, params));
}

/**
* @tc.name: FilterData001
* @tc.desc: test FilterData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, FilterData001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::vector<StoreMetaData> metas = {};
    std::map<std::string, std::vector<std::string>> filterInfo = {};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.FilterData(metas, filterInfo));
}

/**
* @tc.name: FilterData002
* @tc.desc: test FilterData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, FilterData002, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    StoreMetaData storeMeta;
    storeMeta.user = "user";
    std::vector<StoreMetaData> metas = {storeMeta};
    std::map<std::string, std::vector<std::string>> filterInfo;
    filterInfo["USER_INFO"].push_back("");
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.FilterData(metas, filterInfo));
    EXPECT_TRUE(kvStoreDataServiceTest.IsExist("USER_INFO", filterInfo, storeMeta.user));
}

/**
* @tc.name: FilterData003
* @tc.desc: test FilterData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, FilterData003, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    StoreMetaData storeMeta;
    storeMeta.bundleName = "bundleName";
    std::vector<StoreMetaData> metas = {storeMeta};
    std::map<std::string, std::vector<std::string>> filterInfo;

    filterInfo["BUNDLE_INFO"] = {};
    EXPECT_FALSE(kvStoreDataServiceTest.IsExist("BUNDLE_INFO", filterInfo, storeMeta.bundleName));

    filterInfo["BUNDLE_INFO"].push_back("");
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.FilterData(metas, filterInfo));
    EXPECT_TRUE(kvStoreDataServiceTest.IsExist("BUNDLE_INFO", filterInfo, storeMeta.bundleName));
}

/**
* @tc.name: FilterData004
* @tc.desc: test FilterData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, FilterData004, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    StoreMetaData storeMeta;
    storeMeta.storeId = "storeId";
    std::vector<StoreMetaData> metas = {storeMeta};
    std::map<std::string, std::vector<std::string>> filterInfo;
    filterInfo["STORE_INFO"].push_back("");
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.FilterData(metas, filterInfo));
    EXPECT_TRUE(kvStoreDataServiceTest.IsExist("STORE_INFO", filterInfo, storeMeta.storeId));

    filterInfo["STORE_INFO"].push_back("storeId");
    EXPECT_FALSE(kvStoreDataServiceTest.IsExist("STORE_INFO", filterInfo, storeMeta.storeId));
}

/**
* @tc.name: PrintfInfo001
* @tc.desc: test PrintfInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, PrintfInfo001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    StoreMetaData storeMeta1;
    std::vector<StoreMetaData> metas = {storeMeta1};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, metas));

    StoreMetaData storeMeta2;
    metas = {storeMeta1, storeMeta2};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, metas));
}

/**
* @tc.name: PrintfInfo002
* @tc.desc: test PrintfInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, PrintfInfo002, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, KvStoreDataService::UserInfo> datas;
    KvStoreDataService::UserInfo userInfo;
    datas["user1"] = userInfo;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, datas));

    datas["user2"] = userInfo;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, datas));
}

/**
* @tc.name: PrintfInfo003
* @tc.desc: test PrintfInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, PrintfInfo003, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, KvStoreDataService::BundleInfo> datas;
    KvStoreDataService::BundleInfo bundleInfo;
    datas["bundleName1"] = bundleInfo;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, datas));

    datas["bundleName2"] = bundleInfo;
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.PrintfInfo(fd, datas));
}

/**
* @tc.name: BuildData001
* @tc.desc: test BuildData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, BuildData001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::map<std::string, KvStoreDataService::UserInfo> datas;
    KvStoreDataService::UserInfo userInfo;
    datas["user"] = userInfo;
    StoreMetaData storeMeta1;
    std::vector<StoreMetaData> metas = {storeMeta1};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.BuildData(datas, metas));

    StoreMetaData storeMeta2;
    storeMeta2.user = "user";
    metas = {storeMeta2};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.BuildData(datas, metas));
}

/**
* @tc.name: BuildData002
* @tc.desc: test BuildData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, BuildData002, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::map<std::string, KvStoreDataService::BundleInfo> datas;
    KvStoreDataService::BundleInfo bundleInfo;
    datas["bundleName"] = bundleInfo;
    StoreMetaData storeMeta1;
    std::vector<StoreMetaData> metas = {storeMeta1};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.BuildData(datas, metas));

    StoreMetaData storeMeta2;
    storeMeta2.bundleName = "bundleName";
    metas = {storeMeta2};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.BuildData(datas, metas));
}

/**
* @tc.name: DumpUserInfo001
* @tc.desc: test DumpUserInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, DumpUserInfo001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.DumpUserInfo(fd, params));
}

/**
* @tc.name: DumpBundleInfo001
* @tc.desc: test DumpBundleInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KvStoreDataServiceTest, DumpBundleInfo001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    int fd = 0;
    std::map<std::string, std::vector<std::string>> params = {};
    EXPECT_NO_FATAL_FAILURE(kvStoreDataServiceTest.DumpBundleInfo(fd, params));
}

/**
* @tc.name: OnExtensionRestore001
* @tc.desc: restore with invalid fd
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore001, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    data.WriteFileDescriptor(-1);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), -1);
}

/**
* @tc.name: OnExtensionRestore002
* @tc.desc: restore with invalid json
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore002, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, "{}"), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);
    std::string cloneInfoStr = "[{\"type\":\"application_selection\",\"detail\"";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), -1);
    close(fd);
}

/**
* @tc.name: OnExtensionRestore003
* @tc.desc: restore with empty backup content
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore003, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, "{}"), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);
    data.WriteString(NORMAL_CLONE_INFO);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), -1);
    close(fd);
}

/**
* @tc.name: OnExtensionRestore004
* @tc.desc: restore with invalid backup item
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore004, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    std::string backupData =
        "{\"infos\":[{\"bundleName\":\"\",\"dbName\":"
        "\"storeId\",\"instanceId\":0,\"storeType\":\"1\",\"user\":\"100\","
        "\"key\":\"9aJQwx3XD3EN7To2j/I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
        "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]},{"
        "\"bundleName\":\"sa1\",\"dbName\":\"\",\"instanceId\":\"0\","
        "\"dbType\":\"1\",\"user\":\"0\",\"key\":\"9aJQwx3XD3EN7To2j/"
        "I9E9MCzn2+6f/bBqFjOPcY+1pRgx/"
        "XI6jXedyuzEEVdwrc\",\"time\":[50,180,137,103,0,0,0,0]}]}";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, backupData), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);
    data.WriteString(NORMAL_CLONE_INFO);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), 0);
    close(fd);
}

/**
* @tc.name: OnExtensionRestore005
* @tc.desc: restore with empty userId
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore005, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"userId\",\"detail\":\"\"}]";
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, NORMAL_BACKUP_DATA), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), -1);
    close(fd);
}

/**
* @tc.name: OnExtensionRestore006
* @tc.desc: restore success
* @tc.type: FUNC
* @tc.require:
* @tc.author: yanhui
*/
HWTEST_F(KvStoreDataServiceTest, OnExtensionRestore006, TestSize.Level0)
{
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    EXPECT_EQ(WriteContentToFile(SECRETKEY_BACKUP_FILE, NORMAL_BACKUP_DATA), 0);
    int32_t fd = open(SECRETKEY_BACKUP_FILE.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    data.WriteFileDescriptor(fd);
    data.WriteString(NORMAL_CLONE_INFO);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("restore", data, reply), 0);
    close(fd);
}

/**
 * @tc.name: OnExtensionBackup001
 * @tc.desc: test OnExtension function backup invalid json
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup001, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    data.WriteString("InvalidJson");
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup002
 * @tc.desc: test OnExtension function backup application empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup002, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\": \"application_selection\",\"detail\": []}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup003
 * @tc.desc: test OnExtension function backup no userInfo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup003, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"application_selection\",\"detail\":[{\"bundleName\":"
        "\"com.example.restore_test\",\"accessTokenId\":536973769}]}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup004
 * @tc.desc: test OnExtension function backup userinfo empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup004, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"application_selection\",\"detail\":[{\"bundleName\":"
        "\"com.example.restore_test\",\"accessTokenId\":536973769}]},{\"type\":"
        "\"userId\",\"detail\":\"\"}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup005
 * @tc.desc: test OnExtension function backup secret key length invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup005, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"application_selection\",\"detail\":[{"
        "\"bundleName\":\"com.example.restore_test\",\"accessTokenId\":"
        "536973769}]},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup006
 * @tc.desc: test OnExtension function backup secret iv length invalid
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup006, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,112,220,107,106,25,231"
        "16\"}},{\"type\":\"application_selection\",\"detail\":[{"
        "\"bundleName\":\"com.example.restore_test\",\"accessTokenId\":"
        "536973769}]},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), -1);
}

/**
 * @tc.name: OnExtensionBackup007
 * @tc.desc: test OnExtension function backup no bundle
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup007, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"application_selection\",\"detail\":[{"
        "\"bundleName\":\"notexistbundle\",\"accessTokenId\":"
        "536973769}]},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), 0);
    UniqueFd fd(reply.ReadFileDescriptor());
    struct stat statBuf;
    EXPECT_TRUE(fstat(fd.Get(), &statBuf) == 0);
    EXPECT_TRUE(statBuf.st_size > 0);

    char buffer[statBuf.st_size + 1];
    EXPECT_TRUE(read(fd.Get(), buffer, statBuf.st_size + 1) > 0);
    std::string secretKeyStr(buffer);
    EXPECT_TRUE(secretKeyStr == "{\"infos\":[]}");
}

/**
 * @tc.name: OnExtensionBackup008
 * @tc.desc: test OnExtension function backup no bundle
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(KvStoreDataServiceTest, OnExtensionBackup008, TestSize.Level0) {
    KvStoreDataService kvStoreDataServiceTest;
    MessageParcel data;
    MessageParcel reply;
    StoreMetaData testMeta;
    testMeta.bundleName = "com.example.restore_test";
    testMeta.storeId = "Source";
    testMeta.user = "100";
    testMeta.area = CryptoManager::Area::EL1;
    testMeta.instanceId = 0;
    testMeta.deviceId =
        DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    testMeta.isEncrypt = true;
    testMeta.dataDir = "TEST_DIR";
    std::vector<uint8_t> sKey{2,   249, 221, 119, 177, 216, 217, 134, 185, 139,
                              114, 38,  140, 64,  165, 35,  77,  169, 0,   226,
                              226, 166, 37,  73,  181, 229, 42,  88,  108, 111,
                              131, 104, 141, 43,  96,  119, 214, 34,  177, 129,
                              233, 96,  98,  164, 87,  115, 187, 170};
    SecretKeyMetaData testSecret;
    CryptoManager::CryptoParams encryptParams = { .area = testMeta.area };
    testSecret.sKey = CryptoManager::GetInstance().Encrypt(sKey, encryptParams);
    testSecret.storeType = 10;
    testSecret.time = std::vector<uint8_t>{233, 39, 137, 103, 0, 0, 0, 0};
    testSecret.nonce = encryptParams.nonce;
    testSecret.area = encryptParams.area;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(testMeta.GetKey(), testMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(testMeta.GetSecretKey(), testSecret, true), true);

    std::string cloneInfoStr =
        "[{\"type\":\"encryption_info\",\"detail\":{\"encryption_symkey\":\"27,"
        "145,118,212,62,156,133,135,50,68,188,239,20,170,227,190,37,142,218,"
        "158,177,32,5,160,13,114,186,141,59,91,44,200\",\"encryption_algname\":"
        "\"AES256\",\"gcmParams_iv\":\"97,160,201,177,46,37,129,18,112,220,107,"
        "106,25,231,15,15,58,85,31,83,123,216,211,2,222,49,122,72,21,251,83,"
        "16\"}},{\"type\":\"application_selection\",\"detail\":[{"
        "\"bundleName\":\"com.example.restore_test\",\"accessTokenId\":"
        "536973769},{\"bundleName\":\"com.example.notexist\",\"accessTokenId\":"
        "536973769}]},{\"type\":\"userId\",\"detail\":\"100\"}]";
    data.WriteString(cloneInfoStr);
    EXPECT_EQ(kvStoreDataServiceTest.OnExtension("backup", data, reply), 0);
    UniqueFd fd(reply.ReadFileDescriptor());
    struct stat statBuf;
    EXPECT_TRUE(fstat(fd.Get(), &statBuf) == 0);
    EXPECT_TRUE(statBuf.st_size > 0);

    char buffer[statBuf.st_size + 1];
    EXPECT_TRUE(read(fd.Get(), buffer, statBuf.st_size + 1) > 0);
    std::string secretKeyStr(buffer);
    SecretKeyBackupData backupData;
    backupData.Unmarshal(DistributedData::Serializable::ToJson(secretKeyStr));

    EXPECT_TRUE(backupData.infos.size() == 1);
}
} // namespace OHOS::Test
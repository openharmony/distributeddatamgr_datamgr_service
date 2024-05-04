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

#include <memory>
#include "kvstore_client_death_observer.h"
#include "bootstrap.h"
#include "gtest/gtest.h"
#include "kvstore_data_service.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS;

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
* @tc.name: ResolveAutoLaunchParamByIdentifier001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, ResolveAutoLaunchParamByIdentifier001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    std::string identifier = "kvstoredataservicetest";
    DistributedDB::AutoLaunchParam param;
    auto status = kvStoreDataServiceTest.ResolveAutoLaunchParamByIdentifier(identifier, param);
    EXPECT_EQ(status, SUCCESS);
}

/**
* @tc.name: ConvertSecurity001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, ConvertSecurity001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    auto object = kvStoreDataServiceTest.ConvertSecurity(0);
    ASSERT_NE(&object, nullptr);
}

/**
* @tc.name: InitNbDbOption001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KvStoreDataServiceTest, InitNbDbOption001, TestSize.Level1)
{
    KvStoreDataService kvStoreDataServiceTest;
    DistributedDB::KvStoreNbDelegate::Option dbOption;
    Options options = {
        .createIfMissing = false,
        .encrypt = true,
        .autoSync = false,
        .securityLevel = 1,
    };
    std::vector<uint8_t> decryptKey;
    DistributedDB::KvStoreNbDelegate::Option dbOptions;
    auto status = kvStoreDataServiceTest.InitNbDbOption(options, decryptKey, dbOptions);
    EXPECT_EQ(status, SUCCESS);
}
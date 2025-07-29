/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "CloudDataMockTest"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cloud/cloud_server.h"
#include "cloud_service_impl.h"
#include "communicator/device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "mock/account_delegate_mock.h"
#include "mock/db_store_mock.h"
#include "network_delegate_mock.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TEST_CLOUD_BUNDLE = "test_cloud_bundleName";
static constexpr const char *TEST_CLOUD_APPID = "test_cloud_appid";
static constexpr const char *TEST_CLOUD_STORE = "test_cloud_store";
static constexpr const char *TEST_CLOUD_ID = "test_cloud_id";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_1 = "test_cloud_database_alias_1";
static constexpr const char *TEST_CLOUD_DATABASE_ALIAS_2 = "test_cloud_database_alias_2";
static constexpr const char *TEST_CLOUD_PATH = "/data/app/el2/100/database/test_cloud_bundleName/entry/rdb/"
                                               "test_cloud_store";
class CloudDataMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static SchemaMeta schemaMeta_;
    static std::shared_ptr<CloudData::CloudServiceImpl> cloudServiceImpl_;

protected:
    static void InitMetaData();
    static void InitSchemaMeta();
    static void InitCloudInfo();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CloudInfo cloudInfo_;
    static NetworkDelegateMock delegate_;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
};

class CloudServerMock : public CloudServer {
public:
    std::pair<int32_t, CloudInfo> GetServerInfo(int32_t userId, bool needSpaceInfo) override;
    std::pair<int32_t, SchemaMeta> GetAppSchema(int32_t userId, const std::string &bundleName) override;
    virtual ~CloudServerMock() = default;
    static constexpr uint64_t REMAINSPACE = 1000;
    static constexpr uint64_t TATALSPACE = 2000;
    static constexpr int32_t INVALID_USER_ID = -1;
};

std::pair<int32_t, CloudInfo> CloudServerMock::GetServerInfo(int32_t userId, bool needSpaceInfo)
{
    CloudInfo cloudInfo;
    cloudInfo.user = userId;
    cloudInfo.id = TEST_CLOUD_ID;
    cloudInfo.remainSpace = REMAINSPACE;
    cloudInfo.totalSpace = TATALSPACE;
    cloudInfo.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
    return { E_OK, cloudInfo };
}

std::pair<int32_t, SchemaMeta> CloudServerMock::GetAppSchema(int32_t userId, const std::string &bundleName)
{
    if (userId == INVALID_USER_ID) {
        return { E_ERROR, CloudDataMockTest::schemaMeta_ };
    }

    if (bundleName.empty()) {
        SchemaMeta schemaMeta;
        return { E_OK, schemaMeta };
    }
    return { E_OK, CloudDataMockTest::schemaMeta_ };
}

std::shared_ptr<DBStoreMock> CloudDataMockTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
SchemaMeta CloudDataMockTest::schemaMeta_;
StoreMetaData CloudDataMockTest::metaData_;
CloudInfo CloudDataMockTest::cloudInfo_;
std::shared_ptr<CloudData::CloudServiceImpl> CloudDataMockTest::cloudServiceImpl_ =
    std::make_shared<CloudData::CloudServiceImpl>();
NetworkDelegateMock CloudDataMockTest::delegate_;

void CloudDataMockTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_CLOUD_APPID;
    metaData_.bundleName = TEST_CLOUD_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_CLOUD_STORE;
    metaData_.dataDir = TEST_CLOUD_PATH;
    PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
}

void CloudDataMockTest::InitSchemaMeta()
{
    SchemaMeta::Field field1;
    field1.colName = "test_cloud_field_name1";
    field1.alias = "test_cloud_field_alias1";
    SchemaMeta::Field field2;
    field2.colName = "test_cloud_field_name2";
    field2.alias = "test_cloud_field_alias2";

    SchemaMeta::Table table;
    table.name = "test_cloud_table_name";
    table.alias = "test_cloud_table_alias";
    table.fields.emplace_back(field1);
    table.fields.emplace_back(field2);

    SchemaMeta::Database database;
    database.name = TEST_CLOUD_STORE;
    database.alias = TEST_CLOUD_DATABASE_ALIAS_1;
    database.tables.emplace_back(table);

    schemaMeta_.version = 1;
    schemaMeta_.bundleName = TEST_CLOUD_BUNDLE;
    schemaMeta_.databases.emplace_back(database);
    database.alias = TEST_CLOUD_DATABASE_ALIAS_2;
    schemaMeta_.databases.emplace_back(database);
    schemaMeta_.e2eeEnable = false;
}

void CloudDataMockTest::InitCloudInfo()
{
    cloudInfo_.user = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    cloudInfo_.id = TEST_CLOUD_ID;
    cloudInfo_.enableCloud = true;

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = TEST_CLOUD_BUNDLE;
    appInfo.appId = TEST_CLOUD_APPID;
    appInfo.version = 1;
    appInfo.cloudSwitch = true;

    cloudInfo_.apps[TEST_CLOUD_BUNDLE] = std::move(appInfo);
}

void CloudDataMockTest::SetUpTestCase(void)
{
    accountDelegateMock = new (std::nothrow) AccountDelegateMock();
    if (accountDelegateMock != nullptr) {
        AccountDelegate::instance_ = nullptr;
        AccountDelegate::RegisterAccountInstance(accountDelegateMock);
    }
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer(
        [](const auto &, auto) { DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK); });

    auto cloudServerMock = new CloudServerMock();
    CloudServer::RegisterCloudInstance(cloudServerMock);
    size_t max = 12;
    size_t min = 5;

    auto executor = std::make_shared<ExecutorPool>(max, min);
    cloudServiceImpl_->OnBind(
        { "CloudDataMockTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    auto dmExecutor = std::make_shared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    NetworkDelegate::RegisterNetworkInstance(&delegate_);
    delegate_.isNetworkAvailable_ = true;
    InitCloudInfo();
    InitMetaData();
    InitSchemaMeta();
}

void CloudDataMockTest::TearDownTestCase()
{
    if (accountDelegateMock != nullptr) {
        delete accountDelegateMock;
        accountDelegateMock = nullptr;
    }
}

void CloudDataMockTest::SetUp()
{
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetKey(), cloudInfo_, true);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true);
    StoreMetaMapping storeMetaMapping(metaData_);
    MetaDataManager::GetInstance().SaveMeta(storeMetaMapping.GetKey(), storeMetaMapping, true);
    MetaDataManager::GetInstance().SaveMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta_, true);
}

void CloudDataMockTest::TearDown()
{
    EventCenter::GetInstance().Unsubscribe(CloudEvent::LOCAL_CHANGE);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
    StoreMetaMapping storeMetaMapping(metaData_);
    MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(cloudInfo_.GetSchemaKey(TEST_CLOUD_BUNDLE), true);
}

/**
* @tc.name: GetSchema001
* @tc.desc: Test the scenario where the QueryUsers users not empty and return true in the GetSchema function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataMockTest, GetSchema001, TestSize.Level1)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));
    std::vector<int> users = { 0, 1 };
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(true)));
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).Times(1).WillOnce(DoAll(Return(true)));
    DistributedData::StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE,
        0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
    EventCenter::GetInstance().PostEvent(std::move(event));
    auto ret = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true);
    ASSERT_TRUE(ret);
}

/**
* @tc.name: GetSchema002
* @tc.desc: Test the scenario where the QueryUsers users empty in the GetSchema function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataMockTest, GetSchema002, TestSize.Level1)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));

    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_))
        .Times(1)
        .WillOnce(
            DoAll(SetArgReferee<0>(users), Invoke([](std::vector<int> &users) { users.clear(); }), Return(true)));
    DistributedData::StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE,
        0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
    EventCenter::GetInstance().PostEvent(std::move(event));
    auto ret = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true);
    ASSERT_FALSE(ret);
}

/**
* @tc.name: GetSchema003
* @tc.desc: Test the scenario where the QueryUsers return false in the GetSchema function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudDataMockTest, GetSchema003, TestSize.Level1)
{
    auto cloudServerMock = std::make_shared<CloudServerMock>();
    auto user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = cloudServerMock->GetServerInfo(user, true);
    ASSERT_TRUE(MetaDataManager::GetInstance().DelMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), true));
    SchemaMeta schemaMeta;
    ASSERT_FALSE(MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true));

    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(false)));
    DistributedData::StoreInfo storeInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TEST_CLOUD_BUNDLE, TEST_CLOUD_STORE,
        0 };
    auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
    EventCenter::GetInstance().PostEvent(std::move(event));
    auto ret = MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetSchemaKey(TEST_CLOUD_BUNDLE), schemaMeta, true);
    ASSERT_FALSE(ret);
}

/**
* @tc.name: OnReadyTest_LoginAccount
* @tc.desc: Test OnReady function when IsLoginAccount is true or false
* @tc.type: FUNC
* @tc.require:
 */
HWTEST_F(CloudDataMockTest, OnReadyTest_LoginAccount, TestSize.Level0)
{
    ZLOGI("CloudDataMockTest OnReadyTest_LoginAccount start");
    std::string device = "test";
    auto ret = cloudServiceImpl_->OnReady(device);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);

    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).Times(1).WillOnce(testing::Return(false));
    ret = cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    EXPECT_NE(ret, CloudData::CloudService::SUCCESS);

    EXPECT_CALL(*accountDelegateMock, IsLoginAccount()).Times(1).WillOnce(testing::Return(true));
    ret = cloudServiceImpl_->OnReady(DeviceManagerAdapter::CLOUD_DEVICE_UUID);
    EXPECT_EQ(ret, CloudData::CloudService::SUCCESS);
    ZLOGI("CloudDataMockTest OnReadyTest_LoginAccount end");
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
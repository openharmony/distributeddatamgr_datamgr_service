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
#define LOG_TAG "KvdbServiceImplTest"
#include <gtest/gtest.h>
#include "log_print.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "distributed_kv_data_manager.h"
#include "device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "kvdb_service_impl.h"
#include "kvdb_service_stub.h"
#include "kvstore_death_recipient.h"
#include "kvstore_meta_manager.h"
#include "utils/constant.h"
#include "utils/anonymous.h"
#include "token_setproc.h"
#include "types.h"
#include <vector>

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using Action = OHOS::DistributedData::MetaDataManager::Action;
using AppId = OHOS::DistributedKv::AppId;
using ChangeType = OHOS::DistributedData::DeviceMatrix::ChangeType;
using DistributedKvDataManager = OHOS::DistributedKv::DistributedKvDataManager;
using DBStatus = DistributedDB::DBStatus;
using DBMode = DistributedDB::SyncMode;
using Options = OHOS::DistributedKv::Options;
using Status = OHOS::DistributedKv::Status;
using SingleKvStore = OHOS::DistributedKv::SingleKvStore;
using StoreId = OHOS::DistributedKv::StoreId;
using SyncInfo = OHOS::DistributedKv::KVDBService::SyncInfo;
using SyncMode = OHOS::DistributedKv::SyncMode;
using SyncAction = OHOS::DistributedKv::KVDBServiceImpl::SyncAction;
using SwitchState = OHOS::DistributedKv::SwitchState;
using UserId = OHOS::DistributedKv::UserId;
static OHOS::DistributedKv::StoreId storeId = { "kvdb_test_storeid" };
static OHOS::DistributedKv::AppId appId = { "ohos.test.kvdb" };

namespace OHOS::Test {
namespace DistributedDataTest {
class KvdbServiceImplTest : public testing::Test {
public:
    static constexpr size_t NUM_MIN = 5;
    static constexpr size_t NUM_MAX = 12;
    static DistributedKvDataManager manager;
    static Options create;
    static UserId userId;

    std::shared_ptr<SingleKvStore> kvStore;

    static AppId appId;
    static StoreId storeId64;
    static StoreId storeId65;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void RemoveAllStore(OHOS::DistributedKv::DistributedKvDataManager &manager);
    void SetUp();
    void TearDown();

    KvdbServiceImplTest();
protected:
    std::shared_ptr<DistributedKv::KVDBServiceImpl> kvdbServiceImpl_;
};

OHOS::DistributedKv::DistributedKvDataManager KvdbServiceImplTest::manager;
Options KvdbServiceImplTest::create;
UserId KvdbServiceImplTest::userId;

AppId KvdbServiceImplTest::appId;
StoreId KvdbServiceImplTest::storeId64;
StoreId KvdbServiceImplTest::storeId65;

void KvdbServiceImplTest::RemoveAllStore(DistributedKvDataManager &manager)
{
    manager.CloseAllKvStore(appId);
    manager.DeleteAllKvStore(appId, create.baseDir);
}

void KvdbServiceImplTest::SetUpTestCase(void)
{
    auto executors = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    manager.SetExecutors(executors);
    userId.userId = "kvdbserviceimpltest1";
    appId.appId = "ohos.kvdbserviceimpl.test";
    create.createIfMissing = true;
    create.encrypt = false;
    create.securityLevel = OHOS::DistributedKv::S1;
    create.autoSync = true;
    create.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    create.area = OHOS::DistributedKv::EL1;
    create.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(create.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    storeId64.storeId = "a000000000b000000000c000000000d000000000e000000000f000000000g000";
    storeId65.storeId = "a000000000b000000000c000000000d000000000e000000000f000000000g000"
                        "a000000000b000000000c000000000d000000000e000000000f000000000g0000";
    RemoveAllStore(manager);
}

void KvdbServiceImplTest::TearDownTestCase()
{
    RemoveAllStore(manager);
    (void)remove((create.baseDir + "/kvdb").c_str());
    (void)remove(create.baseDir.c_str());
}

void KvdbServiceImplTest::SetUp(void)
{
    kvdbServiceImpl_ = std::make_shared<DistributedKv::KVDBServiceImpl>();
}

void KvdbServiceImplTest::TearDown(void)
{
    RemoveAllStore(manager);
}

KvdbServiceImplTest::KvdbServiceImplTest(void)
{}

/**
* @tc.name: KvdbServiceImpl001
* @tc.desc: KvdbServiceImplTest function test.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, KvdbServiceImpl001, TestSize.Level0)
{
    std::string device = "OH_device_test";
    StoreId id1;
    id1.storeId = "id1";
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    EXPECT_NE(kvStore, nullptr);
    EXPECT_EQ(status, Status::SUCCESS);
    int32_t result = kvdbServiceImpl_->OnInitialize();
    EXPECT_EQ(result, Status::SUCCESS);
    FeatureSystem::Feature::BindInfo bindInfo;
    result = kvdbServiceImpl_->OnBind(bindInfo);
    EXPECT_EQ(result, Status::SUCCESS);
    result = kvdbServiceImpl_->Online(device);
    EXPECT_EQ(result, Status::SUCCESS);
    status = kvdbServiceImpl_->SubscribeSwitchData(appId);
    EXPECT_EQ(status, Status::SUCCESS);
    SyncInfo syncInfo;
    status = kvdbServiceImpl_->CloudSync(appId, id1, syncInfo);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    status = kvdbServiceImpl_->SyncExt(appId, id1, syncInfo);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    syncInfo.devices = {"device1", "device2"};
    syncInfo.query = "query";
    status = kvdbServiceImpl_->SyncExt(appId, id1, syncInfo);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);

    DistributedKv::StoreConfig storeConfig;
    status = kvdbServiceImpl_->SetConfig(appId, id1, storeConfig);
    EXPECT_EQ(status, Status::SUCCESS);
    status = kvdbServiceImpl_->NotifyDataChange(appId, id1, 0);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);

    status = kvdbServiceImpl_->UnsubscribeSwitchData(appId);
    EXPECT_EQ(status, Status::SUCCESS);
    status = kvdbServiceImpl_->Close(appId, id1);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetStoreIdsTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetStoreIdsTest001, TestSize.Level0)
{
    ZLOGI("GetStoreIdsTest001 start");
    StoreId id1;
    id1.storeId = "id1";
    StoreId id2;
    id2.storeId = "id2";
    StoreId id3;
    id3.storeId = "id3";
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = manager.GetSingleKvStore(create, appId, id2, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = manager.GetSingleKvStore(create, appId, id3, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    std::vector<StoreId> storeIds;
    status = kvdbServiceImpl_->GetStoreIds(appId, storeIds);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetStoreIdsTest002
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetStoreIdsTest002, TestSize.Level0)
{
    ZLOGI("GetStoreIdsTest002 start");
    std::vector<StoreId> storeIds;
    AppId appId01;
    auto status = kvdbServiceImpl_->GetStoreIds(appId01, storeIds);
    ZLOGI("GetStoreIdsTest002 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DeleteTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, DeleteTest001, TestSize.Level0)
{
    ZLOGI("DeleteTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    auto status = kvdbServiceImpl_->Delete(appId, storeId);
    ZLOGI("DeleteTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DeleteTest002
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, DeleteTest002, TestSize.Level0)
{
    ZLOGI("DeleteTest002 start");
    AppId appId01 = { "ohos.kvdbserviceimpl.test01" };
    StoreId storeId01 = { "meta_test_storeid" };
    auto status = kvdbServiceImpl_->Delete(appId01, storeId01);
    ZLOGI("DeleteTest002 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: syncTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, syncTest001, TestSize.Level0)
{
    ZLOGI("syncTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->Sync(appId, storeId, syncInfo);
    ZLOGI("syncTest001 status = :%{public}d", status);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: RegisterSyncCallbackTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, RegisterSyncCallbackTest001, TestSize.Level0)
{
    ZLOGI("RegisterSyncCallbackTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    sptr<OHOS::DistributedKv::IKVDBNotifier> notifier;
    auto status = kvdbServiceImpl_->RegServiceNotifier(appId, notifier);
    ZLOGI("RegisterSyncCallbackTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: UnregisterSyncCallbackTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, UnregisterSyncCallbackTest001, TestSize.Level0)
{
    ZLOGI("UnregisterSyncCallbackTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    auto status = kvdbServiceImpl_->UnregServiceNotifier(appId);
    ZLOGI("UnregisterSyncCallbackTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SetSyncParamTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, SetSyncParamTest001, TestSize.Level0)
{
    ZLOGI("SetSyncParamTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    OHOS::DistributedKv::KvSyncParam const syncparam;
    auto status = kvdbServiceImpl_->SetSyncParam(appId, storeId, syncparam);
    ZLOGI("SetSyncParamTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetSyncParamTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetSyncParamTest001, TestSize.Level0)
{
    ZLOGI("GetSyncParamTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    OHOS::DistributedKv::KvSyncParam syncparam;
    auto status = kvdbServiceImpl_->GetSyncParam(appId, storeId, syncparam);
    ZLOGI("GetSyncParamTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: EnableCapabilityTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, EnableCapabilityTest001, TestSize.Level0)
{
    ZLOGI("EnableCapabilityTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    auto status = kvdbServiceImpl_->EnableCapability(appId, storeId);
    ZLOGI("EnableCapabilityTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DisableCapabilityTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, DisableCapabilityTest001, TestSize.Level0)
{
    ZLOGI("DisableCapabilityTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    auto status = kvdbServiceImpl_->DisableCapability(appId, storeId);
    ZLOGI("DisableCapabilityTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SetCapabilityTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, SetCapabilityTest001, TestSize.Level0)
{
    ZLOGI("SetCapabilityTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    std::vector<std::string> local;
    std::vector<std::string> remote;
    auto status = kvdbServiceImpl_->SetCapability(appId, storeId, local, remote);
    ZLOGI("SetCapabilityTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: AddSubscribeInfoTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, AddSubscribeInfoTest001, TestSize.Level0)
{
    ZLOGI("AddSubscribeInfoTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->AddSubscribeInfo(appId, storeId, syncInfo);
    ZLOGI("AddSubscribeInfoTest001 status = :%{public}d", status);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: RmvSubscribeInfoTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, RmvSubscribeInfoTest001, TestSize.Level0)
{
    ZLOGI("RmvSubscribeInfoTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->RmvSubscribeInfo(appId, storeId, syncInfo);
    ZLOGI("RmvSubscribeInfoTest001 status = :%{public}d", status);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: SubscribeTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, SubscribeTest001, TestSize.Level0)
{
    ZLOGI("SubscribeTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    sptr<OHOS::DistributedKv::IKvStoreObserver> observer;
    auto status = kvdbServiceImpl_->Subscribe(appId, storeId, observer);
    ZLOGI("SubscribeTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: UnsubscribeTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, UnsubscribeTest001, TestSize.Level0)
{
    ZLOGI("UnsubscribeTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    sptr<OHOS::DistributedKv::IKvStoreObserver> observer;
    auto status = kvdbServiceImpl_->Unsubscribe(appId, storeId, observer);
    ZLOGI("UnsubscribeTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetBackupPasswordTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetBackupPasswordTest001, TestSize.Level0)
{
    ZLOGI("GetBackupPasswordTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    std::vector<uint8_t> password;
    auto status = kvdbServiceImpl_->GetBackupPassword(appId, storeId, password);
    ZLOGI("GetBackupPasswordTest001 status = :%{public}d", status);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: BeforeCreateTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, BeforeCreateTest001, TestSize.Level0)
{
    ZLOGI("BeforeCreateTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    auto status = kvdbServiceImpl_->BeforeCreate(appId, storeId, create);
    ZLOGI("BeforeCreateTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: AfterCreateTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, AfterCreateTest001, TestSize.Level0)
{
    ZLOGI("AfterCreateTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    std::vector<uint8_t> password;
    auto status = kvdbServiceImpl_->AfterCreate(appId, storeId, create, password);
    ZLOGI("AfterCreateTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: OnAppExitTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, OnAppExitTest001, TestSize.Level0)
{
    ZLOGI("OnAppExitTest001 start");
    Status status1 = manager.GetSingleKvStore(create, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status1, Status::SUCCESS);
    pid_t uid = 1;
    pid_t pid = 2;
    const char **perms = new const char *[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint32_t tokenId = GetAccessTokenId(&infoInstance);
    auto status = kvdbServiceImpl_->OnAppExit(uid, pid, tokenId, appId);
    ZLOGI("OnAppExitTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: OnUserChangeTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, OnUserChangeTest001, TestSize.Level0)
{
    ZLOGI("OnUserChangeTest001 start");
    uint32_t code = 1;
    std::string user = "OH_USER_test";
    std::string account = "OH_ACCOUNT_test";
    auto status = kvdbServiceImpl_->OnUserChange(code, user, account);
    ZLOGI("OnUserChangeTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: OnReadyTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, OnReadyTest001, TestSize.Level0)
{
    ZLOGI("OnReadyTest001 start");
    std::string device = "OH_device_test";
    auto status = kvdbServiceImpl_->OnReady(device);
    ZLOGI("OnReadyTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
    status = kvdbServiceImpl_->OnSessionReady(device);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: ResolveAutoLaunch
* @tc.desc: ResolveAutoLaunch function test.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ResolveAutoLaunch, TestSize.Level0)
{
    StoreId id1;
    id1.storeId = "id1";
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    EXPECT_NE(kvStore, nullptr);
    EXPECT_EQ(status, Status::SUCCESS);
    std::string identifier = "identifier";
    DistributedKv::KVDBServiceImpl::DBLaunchParam launchParam;
    auto result = kvdbServiceImpl_->ResolveAutoLaunch(identifier, launchParam);
    EXPECT_EQ(result, Status::STORE_NOT_FOUND);
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 0);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    result = kvdbServiceImpl_->ResolveAutoLaunch(identifier, launchParam);
    EXPECT_EQ(result, Status::SUCCESS);
}

/**
* @tc.name: PutSwitch
* @tc.desc: PutSwitch function test.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, PutSwitch, TestSize.Level0)
{
    StoreId id1;
    id1.storeId = "id1";
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    DistributedKv::SwitchData switchData;
    status = kvdbServiceImpl_->PutSwitch(appId, switchData);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    switchData.value = DeviceMatrix::INVALID_VALUE;
    switchData.length = DeviceMatrix::INVALID_LEVEL;
    status = kvdbServiceImpl_->PutSwitch(appId, switchData);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    switchData.value = DeviceMatrix::INVALID_MASK;
    switchData.length = DeviceMatrix::INVALID_LENGTH;
    status = kvdbServiceImpl_->PutSwitch(appId, switchData);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    switchData.value = DeviceMatrix::INVALID_VALUE;
    switchData.length = DeviceMatrix::INVALID_LENGTH;
    status = kvdbServiceImpl_->PutSwitch(appId, switchData);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    switchData.value = DeviceMatrix::INVALID_MASK;
    switchData.length = DeviceMatrix::INVALID_LEVEL;
    status = kvdbServiceImpl_->PutSwitch(appId, switchData);
    EXPECT_EQ(status, Status::SUCCESS);
    std::string networkId = "networkId";
    status = kvdbServiceImpl_->GetSwitch(appId, networkId, switchData);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: DoCloudSync
* @tc.desc: DoCloudSync error function test.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, DoCloudSync, TestSize.Level0)
{
    StoreId id1;
    id1.storeId = "id1";
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    StoreMetaData metaData;
    SyncInfo syncInfo;
    status = kvdbServiceImpl_->DoCloudSync(metaData, syncInfo);
    EXPECT_EQ(status, Status::NOT_SUPPORT);
    syncInfo.devices = {"device1", "device2"};
    syncInfo.query = "query";
    metaData.enableCloud = true;
    status = kvdbServiceImpl_->DoCloudSync(metaData, syncInfo);
    EXPECT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: ConvertDbStatus
* @tc.desc: ConvertDbStatus test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ConvertDbStatus, TestSize.Level0)
{
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::BUSY), Status::DB_ERROR);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::DB_ERROR), Status::DB_ERROR);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::OK), Status::SUCCESS);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_ARGS), Status::INVALID_ARGUMENT);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::NOT_FOUND), Status::KEY_NOT_FOUND);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_VALUE_FIELDS), Status::INVALID_VALUE_FIELDS);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_FIELD_TYPE), Status::INVALID_FIELD_TYPE);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::CONSTRAIN_VIOLATION), Status::CONSTRAIN_VIOLATION);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_FORMAT), Status::INVALID_FORMAT);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_QUERY_FORMAT), Status::INVALID_QUERY_FORMAT);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::INVALID_QUERY_FIELD), Status::INVALID_QUERY_FIELD);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::NOT_SUPPORT), Status::NOT_SUPPORT);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::TIME_OUT), Status::TIME_OUT);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::OVER_MAX_LIMITS), Status::OVER_MAX_LIMITS);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::EKEYREVOKED_ERROR), Status::SECURITY_LEVEL_ERROR);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::SECURITY_OPTION_CHECK_ERROR), Status::SECURITY_LEVEL_ERROR);
    EXPECT_EQ(kvdbServiceImpl_->ConvertDbStatus(DBStatus::SCHEMA_VIOLATE_VALUE), Status::ERROR);
}

/**
* @tc.name: ConvertDBMode
* @tc.desc: ConvertDBMode test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ConvertDBMode, TestSize.Level0)
{
    auto status = kvdbServiceImpl_->ConvertDBMode(SyncMode::PUSH);
    EXPECT_EQ(status, DBMode::SYNC_MODE_PUSH_ONLY);
    status = kvdbServiceImpl_->ConvertDBMode(SyncMode::PULL);
    EXPECT_EQ(status, DBMode::SYNC_MODE_PULL_ONLY);
    status = kvdbServiceImpl_->ConvertDBMode(SyncMode::PUSH_PULL);
    EXPECT_EQ(status, DBMode::SYNC_MODE_PUSH_PULL);
}

/**
* @tc.name: ConvertGeneralSyncMode
* @tc.desc: ConvertGeneralSyncMode test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ConvertGeneralSyncMode, TestSize.Level0)
{
    auto status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PUSH, SyncAction::ACTION_SUBSCRIBE);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE);
    status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PUSH, SyncAction::ACTION_UNSUBSCRIBE);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_UNSUBSCRIBE_REMOTE);
    status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PUSH, SyncAction::ACTION_SYNC);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_PUSH);
    status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PULL, SyncAction::ACTION_SYNC);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_PULL);
    status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PUSH_PULL, SyncAction::ACTION_SYNC);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_PULL_PUSH);
    auto action = static_cast<SyncAction>(SyncAction::ACTION_UNSUBSCRIBE + 1);
    status = kvdbServiceImpl_->ConvertGeneralSyncMode(SyncMode::PUSH, action);
    EXPECT_EQ(status, GeneralStore::SyncMode::NEARBY_END);
}

/**
* @tc.name: ConvertType
* @tc.desc: ConvertType test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ConvertType, TestSize.Level0)
{
    auto status = kvdbServiceImpl_->ConvertType(SyncMode::PUSH);
    EXPECT_EQ(status, ChangeType::CHANGE_LOCAL);
    status = kvdbServiceImpl_->ConvertType(SyncMode::PULL);
    EXPECT_EQ(status, ChangeType::CHANGE_REMOTE);
    status = kvdbServiceImpl_->ConvertType(SyncMode::PUSH_PULL);
    EXPECT_EQ(status, ChangeType::CHANGE_ALL);
    auto action = static_cast<SyncMode>(SyncMode::PUSH_PULL + 1);
    status = kvdbServiceImpl_->ConvertType(action);
    EXPECT_EQ(status, ChangeType::CHANGE_ALL);
}

/**
* @tc.name: ConvertAction
* @tc.desc: ConvertAction test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, ConvertAction, TestSize.Level0)
{
    auto status = kvdbServiceImpl_->ConvertAction(Action::INSERT);
    EXPECT_EQ(status, SwitchState::INSERT);
    status = kvdbServiceImpl_->ConvertAction(Action::UPDATE);
    EXPECT_EQ(status, SwitchState::UPDATE);
    status = kvdbServiceImpl_->ConvertAction(Action::DELETE);
    EXPECT_EQ(status, SwitchState::DELETE);
    auto action = static_cast<Action>(Action::DELETE + 1);
    status = kvdbServiceImpl_->ConvertAction(action);
    EXPECT_EQ(status, SwitchState::INSERT);
}

/**
* @tc.name: GetSyncMode
* @tc.desc: GetSyncMode test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvdbServiceImplTest, GetSyncMode, TestSize.Level0)
{
    auto status = kvdbServiceImpl_->GetSyncMode(true, true);
    EXPECT_EQ(status, SyncMode::PUSH_PULL);
    status = kvdbServiceImpl_->GetSyncMode(true, false);
    EXPECT_EQ(status, SyncMode::PUSH);
    status = kvdbServiceImpl_->GetSyncMode(false, true);
    EXPECT_EQ(status, SyncMode::PULL);
    status = kvdbServiceImpl_->GetSyncMode(false, false);
    EXPECT_EQ(status, SyncMode::PUSH_PULL);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
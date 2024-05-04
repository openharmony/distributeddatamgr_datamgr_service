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
#define LOG_TAG "KvdbServiceImplTest"
#include <gtest/gtest.h>
#include "log_print.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"

#include "kvdb_service_stub.h"
#include "checker/checker_manager.h"
#include "utils/constant.h"
#include "utils/anonymous.h"
#include "distributed_kv_data_manager.h"

#include <vector>
#include "kvstore_death_recipient.h"
#include "types.h"
#include "kvdb_service_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

using Status = OHOS::DistributedKv::Status;
using Options = OHOS::DistributedKv::Options;
using SingleKvStore = OHOS::DistributedKv::SingleKvStore;
using DistributedKvDataManager = OHOS::DistributedKv::DistributedKvDataManager;
using UserId = OHOS::DistributedKv::UserId;

using StoreId = OHOS::DistributedKv::StoreId;
using AppId = OHOS::DistributedKv::AppId;
using SyncInfo = OHOS::DistributedKv::KVDBService::SyncInfo;
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
    ASSERT_NE(status, Status::SUCCESS);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test

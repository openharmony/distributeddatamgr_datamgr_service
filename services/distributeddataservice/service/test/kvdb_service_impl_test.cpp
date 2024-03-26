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
#include "ipc_skeleton.h"
#include "device_matrix.h"
#include "executor_pool.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "kvstore_meta_manager.h"
#include "device_manager_adapter.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "kvdb_service_impl.h"
#include "metadata/store_meta_data_local.h"
#include "../cloud/sync_manager.h"
#include "kvdb_service_stub.h"
#include "checker/checker_manager.h"
#include "utils/constant.h"
#include "utils/anonymous.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Status = OHOS::DistributedKv::Status;
using Options = OHOS::DistributedKv::Options;
using SyncInfo = OHOS::DistributedKv::KVDBService::SyncInfo;
static OHOS::DistributedKv::StoreId storeId = { "meta_test_storeid" };
static OHOS::DistributedKv::AppId appId = { "ohos.test.kvdb" };
static constexpr const char *TEST_USER = "0";
namespace OHOS::Test {
namespace DistributedDataTest {
class KvdbServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    StoreMetaData metaData_;
    Options options_;
    std::shared_ptr<DistributedKv::KVDBServiceImpl> kvdbServiceImpl_;
    int32_t GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId);
};

int32_t KvdbServiceImplTest::GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        return -1;
    }
    return tokenInfo.instIndex;
}

void GrantPermissionNative()
{
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
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete []perms;
}

void KvdbServiceImplTest::SetUpTestCase(void)
{
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    GrantPermissionNative();

    size_t max = 12;
    size_t min = 5;
    auto executors = std::make_shared<OHOS::ExecutorPool>(max, min);
    DmAdapter::GetInstance().Init(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
}

void KvdbServiceImplTest::TearDownTestCase() {}

void KvdbServiceImplTest::SetUp()
{
    options_.isNeedCompress = true;
    kvdbServiceImpl_ = std::make_shared<DistributedKv::KVDBServiceImpl>();
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.bundleName = appId.appId;
    metaData_.storeId = storeId.storeId;
    metaData_.user = TEST_USER;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.instanceId = GetInstIndex(metaData_.tokenId, appId);
    metaData_.version = 1;
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey());
}

void KvdbServiceImplTest::TearDown() {}

/**
* @tc.name: GetStoreIdsTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetStoreIdsTest001, TestSize.Level0)
{

    ZLOGI("GetStoreIdsTest001 start");
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    auto status = kvdbServiceImpl_->GetStoreIds(appId, storeIds);
    ZLOGI("GetStoreIdsTest001 status = :%{public}d", status);
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
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    OHOS::DistributedKv::AppId appId02;
    auto status = kvdbServiceImpl_->GetStoreIds(appId02, storeIds);
    ZLOGI("GetStoreIdsTest002 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS); 
}

/**
* @tc.name: GetStoreIdsTest003
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetStoreIdsTest003, TestSize.Level0)
{
    ZLOGI("GetStoreIdsTest003 start");
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    OHOS::DistributedKv::StoreId storeid1;
    OHOS::DistributedKv::StoreId storeid2;
    storeid1.storeId = "KvdbServiceImplTest001";
    storeid2.storeId = "KvdbServiceImplTest002";
    storeIds.emplace_back(storeid1);
    storeIds.emplace_back(storeid2);
    storeIds.emplace_back(storeId);
    OHOS::DistributedKv::AppId appId03;
    auto status = kvdbServiceImpl_->GetStoreIds(appId03, storeIds);
    ZLOGI("GetStoreIdsTest003 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetStoreIdsTest004
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, GetStoreIdsTest004, TestSize.Level0)
{
    ZLOGI("GetStoreIdsTest004 start");
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    OHOS::DistributedKv::StoreId storeid1;
    OHOS::DistributedKv::StoreId storeid2;
    storeid1.storeId = "KvdbServiceImplTest003";
    storeid2.storeId = "KvdbServiceImplTest004";
    storeIds.emplace_back(storeid1);
    storeIds.emplace_back(storeid2);
    storeIds.emplace_back(storeId);
    auto status = kvdbServiceImpl_->GetStoreIds(appId, storeIds);
    ZLOGI("GetStoreIdsTest004 status = :%{public}d", status);
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
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    kvdbServiceImpl_->GetStoreIds(appId, storeIds);
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
    std::vector<OHOS::DistributedKv::StoreId> storeIds;
    storeIds.emplace_back(storeId);
    kvdbServiceImpl_->GetStoreIds(appId, storeIds);
    auto status = kvdbServiceImpl_->Delete(appId, storeId);
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
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->Sync(appId, storeId, syncInfo);
    ZLOGI("syncTest001 status = :%{public}d", status);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: syncExtTest001
* @tc.desc: GetStoreIds
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(KvdbServiceImplTest, syncExtTest001, TestSize.Level0)
{
    ZLOGI("syncExtTest001 start");
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->SyncExt(appId, storeId, syncInfo);
    ZLOGI("syncExtTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
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
    sptr<OHOS::DistributedKv::IKvStoreSyncCallback> callbacks;
    auto status = kvdbServiceImpl_->RegisterSyncCallback(appId, callbacks);
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
    auto status = kvdbServiceImpl_->UnregisterSyncCallback(appId);
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
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->AddSubscribeInfo(appId, storeId, syncInfo);
    ZLOGI("AddSubscribeInfoTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
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
    SyncInfo syncInfo;
    auto status = kvdbServiceImpl_->RmvSubscribeInfo(appId, storeId, syncInfo);
    ZLOGI("RmvSubscribeInfoTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
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
    sptr<OHOS::DistributedKv::IKvStoreObserver> observer;
    auto status = kvdbServiceImpl_->Subscribe(appId, storeId, observer);
    ZLOGI("SubscribeTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
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
    std::vector<uint8_t> password;
    auto status = kvdbServiceImpl_->GetBackupPassword(appId, storeId, password);
    ZLOGI("GetBackupPasswordTest001 status = :%{public}d", status);
    ASSERT_EQ(status, Status::SUCCESS);
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
    auto status = kvdbServiceImpl_->BeforeCreate(appId, storeId, options_);
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
    std::vector<uint8_t> password;
    auto status = kvdbServiceImpl_->AfterCreate(appId, storeId, options_, password);
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
}
} // namespace DistributedDataTest
} // namespace OHOS::Test

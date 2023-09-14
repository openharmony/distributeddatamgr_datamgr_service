/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communicator/device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "mock/general_store_mock.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "mock/db_store_mock.h"
#include "rdb_notifier_stub.h"
#include "rdb_service_impl.h"
#include "store/auto_cache.h"
#include "token_setproc.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
namespace OHOS::Test {
namespace DistributedRDBTest {
constexpr int USER_ID = 100;
constexpr int INST_INDEX = 0;
constexpr const char* BUNDLE_NAME = "ohos.RdbServiceImplTest.BundleName";
constexpr const char* STORE_ID = "RdbServiceImplTestStoreId";
constexpr const char* CHECK_NAME = "SystemChecker";

void AllocAndSetHapToken()
{
    HapInfoParams info = {
        .userID = USER_ID,
        .bundleName = BUNDLE_NAME,
        .instIndex = INST_INDEX,
        .appIDDesc = BUNDLE_NAME,
        .isSystemApp = true
    };

    HapPolicyParams policy = { .apl = APL_NORMAL, .domain = "test.domain", .permList = {}, .permStateList = {} };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}
class TestChecker : public CheckerManager::Checker {
public:
    TestChecker() noexcept
    {
        CheckerManager::GetInstance().RegisterPlugin(
            CHECK_NAME, [this]() -> auto { return this; });
    }
    ~TestChecker() {}
    void Initialize() override {}
    bool SetTrustInfo(const CheckerManager::Trust& trust) override
    {
        return true;
    }
    std::string GetAppId(const CheckerManager::StoreInfo &info) override
    {
        return info.bundleName;
    }
    bool IsValid(const CheckerManager::StoreInfo &info) override
    {
        return true;
    }

private:
    static TestChecker instance_;
};
__attribute__((used)) TestChecker TestChecker::instance_;
class NotifierMock : public RdbNotifierStub {
public:
    explicit NotifierMock(const SyncCompleteHandler& syncCompleteHandler = nullptr,
        const DataChangeHandler& dataChangeHandler = nullptr)
        : RdbNotifierStub(syncCompleteHandler, dataChangeHandler)
    {
    }
    virtual ~NotifierMock() = default;
    int OnComplete(uint32_t, Details&&)
    {
        return RDB_OK;
    }
    int OnChange(const Origin&, const PrimaryFields&, ChangeInfo&&)
    {
        times_++;
        return RDB_OK;
    }
    int GetTimes()
    {
        return times_;
    }

private:
    std::atomic_int times_ = 0;
};

class RdbServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static StoreMetaData GetStoreMetaData();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
};

std::shared_ptr<DBStoreMock> RdbServiceImplTest::dbStoreMock_ = std::make_shared<DBStoreMock>();

StoreMetaData RdbServiceImplTest::GetStoreMetaData()
{
    StoreMetaData storeMetaData;
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaData.bundleName = BUNDLE_NAME;
    storeMetaData.storeId = STORE_ID;
    storeMetaData.appId = BUNDLE_NAME;
    storeMetaData.instanceId = 0;
    storeMetaData.isAutoSync = true;
    storeMetaData.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    storeMetaData.area = OHOS::DistributedKv::EL1;
    storeMetaData.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    storeMetaData.user =
        std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(storeMetaData.tokenId));
    return storeMetaData;
}

void RdbServiceImplTest::SetUpTestCase(void)
{
    AllocAndSetHapToken();
    AutoCache::GetInstance().RegCreator(RDB_DEVICE_COLLABORATION, [](const StoreMetaData& metaData) -> GeneralStore* {
        return new (std::nothrow) GeneralStoreMock(metaData);
    });
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, nullptr);
    std::vector<std::string> checks { CHECK_NAME };
    CheckerManager::GetInstance().LoadCheckers(checks);
    FeatureSystem::GetInstance().GetCreator("relational_store")();
}

void RdbServiceImplTest::TearDownTestCase(void) {}

void RdbServiceImplTest::SetUp() {}

void RdbServiceImplTest::TearDown()
{
    AutoCache::GetInstance().CloseStore(OHOS::IPCSkeleton::GetCallingTokenID());
    if (dbStoreMock_ != nullptr) {
        dbStoreMock_->Reset();
    }
}

/**
* @tc.name: InitNotifierOnce
* @tc.desc: InitNotifierOnce and be able to notify normally
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(RdbServiceImplTest, InitNotifierOnce, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RdbServiceImplTest InitNotifierOnce start";
    std::shared_ptr<RdbServiceImpl> rdbService = std::make_shared<RdbServiceImpl>();
    RdbSyncerParam param;
    param.bundleName_ = BUNDLE_NAME;
    param.storeName_ = STORE_ID;
    sptr<NotifierMock> notifierMock { new NotifierMock() };
    ASSERT_EQ(rdbService->InitNotifier(param, notifierMock), RDB_OK);
    SubscribeOption option;
    option.mode = CLOUD_DETAIL;
    ASSERT_EQ(rdbService->Subscribe(param, option, nullptr), RDB_OK);
    auto storeMeta = GetStoreMetaData();
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKey(), storeMeta));
    auto identifier = DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(storeMeta.user,
        storeMeta.appId, storeMeta.storeId);
    DistributedDB::AutoLaunchParam autoLaunchParam;
    ASSERT_TRUE(rdbService->ResolveAutoLaunch(identifier, autoLaunchParam));
    auto store = std::static_pointer_cast<GeneralStoreMock>(AutoCache::GetInstance().GetStore(storeMeta, {}));
    ASSERT_NE(store, nullptr);
    ASSERT_EQ(store->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 1);
    ASSERT_EQ(rdbService->UnSubscribe(param, option, nullptr), RDB_OK);
    ASSERT_EQ(store->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 1);
    GTEST_LOG_(INFO) << "RdbServiceImplTest InitNotifierOnce end";
}

/**
* @tc.name: SubscribeTwoStores
* @tc.desc: Notify twice when a process subscribes to two stores, and notify once when subscribing to one store.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(RdbServiceImplTest, SubscribeTwoStores, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RdbServiceImplTest SubscribeTwoStores start";
    std::shared_ptr<RdbServiceImpl> rdbService = std::make_shared<RdbServiceImpl>();
    RdbSyncerParam param;
    param.bundleName_ = BUNDLE_NAME;
    sptr<NotifierMock> notifierMock { new NotifierMock() };
    ASSERT_EQ(rdbService->InitNotifier(param, notifierMock), RDB_OK);
    SubscribeOption option;
    option.mode = REMOTE;

    param.storeName_ = std::string(STORE_ID) + "1";
    ASSERT_EQ(rdbService->Subscribe(param, option, nullptr), RDB_OK);
    auto storeMeta = GetStoreMetaData();
    storeMeta.storeId = param.storeName_;
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKey(), storeMeta));
    auto identifier = DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(storeMeta.user,
        storeMeta.appId, storeMeta.storeId);
    DistributedDB::AutoLaunchParam autoLaunchParam;
    ASSERT_TRUE(rdbService->ResolveAutoLaunch(identifier, autoLaunchParam));
    auto store1 = std::static_pointer_cast<GeneralStoreMock>(AutoCache::GetInstance().GetStore(storeMeta, {}));
    ASSERT_NE(store1, nullptr);

    param.storeName_ = std::string(STORE_ID) + "2";
    ASSERT_EQ(rdbService->Subscribe(param, option, nullptr), RDB_OK);
    storeMeta.storeId = param.storeName_;
    MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKey(), storeMeta);
    identifier = DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(storeMeta.user,
        storeMeta.appId, storeMeta.storeId);
    ASSERT_TRUE(rdbService->ResolveAutoLaunch(identifier, autoLaunchParam));
    auto store2 = std::static_pointer_cast<GeneralStoreMock>(AutoCache::GetInstance().GetStore(storeMeta, {}));
    ASSERT_NE(store2, nullptr);

    ASSERT_EQ(store1->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 1);
    ASSERT_EQ(store2->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 2);

    ASSERT_EQ(rdbService->UnSubscribe(param, option, nullptr), RDB_OK);
    ASSERT_EQ(store1->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 3);
    ASSERT_EQ(store2->OnChange(GeneralWatcher::Origin(), {}, {}), GeneralError::E_OK);
    ASSERT_EQ(notifierMock->GetTimes(), 3);
    GTEST_LOG_(INFO) << "RdbServiceImplTest SubscribeTwoStores end";
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
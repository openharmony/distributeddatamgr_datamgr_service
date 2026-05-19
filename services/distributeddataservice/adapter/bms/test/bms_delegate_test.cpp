/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "BmsDelegateTest"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "bms/bms_delegate.h"
#include "bms_delegate_impl.h"
#include "bundlemgr/bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "iremote_stub.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace {
using namespace OHOS;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DistributedData;
using namespace testing;
using namespace testing::ext;

constexpr int32_t USER_ID = 100;
constexpr int32_t ERROR_CODE = -1;
const std::string BUNDLE_NAME = "com.example.datamining";
const std::string APP_IDENTIFIER = "appIdentifier";

class BmsDelegateMock final : public BmsDelegate {
public:
    std::string GetCallerAppIdentifier(const std::string &bundleName, int32_t userId) override
    {
        return bundleName + std::to_string(userId);
    }
};

class BundleMgrMock : public IRemoteStub<IBundleMgr> {
public:
    MOCK_METHOD(ErrCode, GetBundleInfoV9, (const std::string &, int32_t, BundleInfo &, int32_t), (override));
    MOCK_METHOD(bool, AddDeathRecipient, (const sptr<IRemoteObject::DeathRecipient> &), (override));
    MOCK_METHOD(bool, RemoveDeathRecipient, (const sptr<IRemoteObject::DeathRecipient> &), (override));
};

class SystemAbilityManagerMock : public ISystemAbilityManager {
public:
    MOCK_METHOD(sptr<IRemoteObject>, AsObject, (), (override));
    MOCK_METHOD(std::vector<std::u16string>, ListSystemAbilities, (unsigned int), (override));
    MOCK_METHOD(sptr<IRemoteObject>, GetSystemAbility, (int32_t), (override));
    MOCK_METHOD(sptr<IRemoteObject>, CheckSystemAbility, (int32_t), (override));
    MOCK_METHOD(int32_t, RemoveSystemAbility, (int32_t), (override));
    MOCK_METHOD(int32_t, SubscribeSystemAbility, (int32_t, const sptr<ISystemAbilityStatusChange> &), (override));
    MOCK_METHOD(int32_t, UnSubscribeSystemAbility, (int32_t, const sptr<ISystemAbilityStatusChange> &), (override));
    MOCK_METHOD(sptr<IRemoteObject>, GetSystemAbility, (int32_t, const std::string &), (override));
    MOCK_METHOD(sptr<IRemoteObject>, CheckSystemAbility, (int32_t, const std::string &), (override));
    MOCK_METHOD(int32_t, AddOnDemandSystemAbilityInfo, (int32_t, const std::u16string &), (override));
    MOCK_METHOD(sptr<IRemoteObject>, CheckSystemAbility, (int32_t, bool &), (override));
    MOCK_METHOD(int32_t, AddSystemAbility,
        (int32_t, const sptr<IRemoteObject> &, const ISystemAbilityManager::SAExtraProp &), (override));
    MOCK_METHOD(int32_t, AddSystemProcess, (const std::u16string &, const sptr<IRemoteObject> &), (override));
    MOCK_METHOD(sptr<IRemoteObject>, LoadSystemAbility, (int32_t, int32_t), (override));
    MOCK_METHOD(int32_t, LoadSystemAbility, (int32_t, const sptr<ISystemAbilityLoadCallback> &), (override));
    MOCK_METHOD(int32_t, LoadSystemAbility,
        (int32_t, const std::string &, const sptr<ISystemAbilityLoadCallback> &), (override));
    MOCK_METHOD(int32_t, UnloadSystemAbility, (int32_t), (override));
    MOCK_METHOD(int32_t, CancelUnloadSystemAbility, (int32_t), (override));
    MOCK_METHOD(int32_t, UnloadAllIdleSystemAbility, (), (override));
    MOCK_METHOD(int32_t, GetSystemProcessInfo, (int32_t, SystemProcessInfo &), (override));
    MOCK_METHOD(int32_t, GetRunningSystemProcess, (std::list<SystemProcessInfo> &), (override));
    MOCK_METHOD(int32_t, SubscribeSystemProcess, (const sptr<ISystemProcessStatusChange> &), (override));
    MOCK_METHOD(int32_t, SendStrategy, (int32_t, std::vector<int32_t> &, int32_t, std::string &), (override));
    MOCK_METHOD(int32_t, UnSubscribeSystemProcess, (const sptr<ISystemProcessStatusChange> &), (override));
    MOCK_METHOD(int32_t, GetExtensionSaIds, (const std::string &, std::vector<int32_t> &), (override));
    MOCK_METHOD(int32_t, GetExtensionRunningSaList,
        (const std::string &, std::vector<sptr<IRemoteObject>> &), (override));
    MOCK_METHOD(int32_t, GetRunningSaExtensionInfoList,
        (const std::string &, std::vector<ISystemAbilityManager::SaExtensionInfo> &), (override));
    MOCK_METHOD(int32_t, GetCommonEventExtraDataIdlist,
        (int32_t, std::vector<int64_t> &, const std::string &), (override));
    MOCK_METHOD(int32_t, GetOnDemandReasonExtraData, (int64_t, MessageParcel &), (override));
    MOCK_METHOD(int32_t, GetOnDemandPolicy,
        (int32_t, OnDemandPolicyType, std::vector<SystemAbilityOnDemandEvent> &), (override));
    MOCK_METHOD(int32_t, UpdateOnDemandPolicy,
        (int32_t, OnDemandPolicyType, const std::vector<SystemAbilityOnDemandEvent> &), (override));
    MOCK_METHOD(int32_t, GetOnDemandSystemAbilityIds, (std::vector<int32_t> &), (override));
};

class BmsDelegateTest : public testing::Test {
public:
    void SetUp() override
    {
        SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(nullptr);
    }

    void TearDown() override
    {
        SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(nullptr);
        BmsDelegate::RegisterInstance(std::make_shared<BmsDelegateImpl>());
    }
};

/**
* @tc.name: GetInstance001
* @tc.desc: get default BMS delegate when no instance is registered
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetInstance001, TestSize.Level0)
{
    BmsDelegate::RegisterInstance(nullptr);
    auto delegate = BmsDelegate::GetInstance();
    ASSERT_NE(delegate, nullptr);
    EXPECT_TRUE(delegate->GetCallerAppIdentifier(BUNDLE_NAME, USER_ID).empty());
}

/**
* @tc.name: RegisterInstance001
* @tc.desc: register BMS delegate implementation
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, RegisterInstance001, TestSize.Level0)
{
    auto delegate = std::make_shared<BmsDelegateMock>();
    EXPECT_TRUE(BmsDelegate::RegisterInstance(delegate));
    EXPECT_EQ(BmsDelegate::GetInstance(), delegate);
    EXPECT_EQ(BmsDelegate::GetInstance()->GetCallerAppIdentifier(BUNDLE_NAME, USER_ID),
        BUNDLE_NAME + std::to_string(USER_ID));
}

/**
* @tc.name: GetBundleMgr001
* @tc.desc: return null when system ability manager is null
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetBundleMgr001, TestSize.Level0)
{
    BmsDelegateImpl delegate;
    EXPECT_EQ(delegate.GetBundleMgr(), nullptr);
}

/**
* @tc.name: GetBundleMgr002
* @tc.desc: return null when BMS system ability is not ready
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetBundleMgr002, TestSize.Level0)
{
    auto systemAbilityManager = sptr<SystemAbilityManagerMock>(new (std::nothrow) SystemAbilityManagerMock());
    ASSERT_NE(systemAbilityManager, nullptr);
    SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(systemAbilityManager);

    EXPECT_CALL(*systemAbilityManager, GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID))
        .WillOnce(Return(nullptr));
    BmsDelegateImpl delegate;
    EXPECT_EQ(delegate.GetBundleMgr(), nullptr);
}

/**
* @tc.name: GetBundleMgr003
* @tc.desc: return null when death recipient registration fails
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetBundleMgr003, TestSize.Level0)
{
    auto systemAbilityManager = sptr<SystemAbilityManagerMock>(new (std::nothrow) SystemAbilityManagerMock());
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(systemAbilityManager, nullptr);
    ASSERT_NE(bundleMgr, nullptr);
    SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(systemAbilityManager);

    EXPECT_CALL(*systemAbilityManager, GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID))
        .WillOnce(Return(bundleMgr));
    EXPECT_CALL(*bundleMgr, AddDeathRecipient(_)).WillOnce(Return(false));
    BmsDelegateImpl delegate;
    EXPECT_EQ(delegate.GetBundleMgr(), nullptr);
    EXPECT_EQ(delegate.object_, nullptr);
    EXPECT_EQ(delegate.deathRecipient_, nullptr);
}

/**
* @tc.name: GetBundleMgr004
* @tc.desc: return bundle manager when death recipient registration succeeds
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetBundleMgr004, TestSize.Level0)
{
    auto systemAbilityManager = sptr<SystemAbilityManagerMock>(new (std::nothrow) SystemAbilityManagerMock());
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(systemAbilityManager, nullptr);
    ASSERT_NE(bundleMgr, nullptr);
    SystemAbilityManagerClient::GetInstance().SetSystemAbilityManager(systemAbilityManager);

    EXPECT_CALL(*systemAbilityManager, GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID))
        .WillOnce(Return(bundleMgr));
    EXPECT_CALL(*bundleMgr, AddDeathRecipient(_)).WillOnce(Return(true));
    EXPECT_CALL(*bundleMgr, RemoveDeathRecipient(_)).WillOnce(Return(true));
    BmsDelegateImpl delegate;
    sptr<IBundleMgr> expected = iface_cast<IBundleMgr>(bundleMgr);
    EXPECT_EQ(delegate.GetBundleMgr(), expected);
    EXPECT_NE(delegate.deathRecipient_, nullptr);
}

/**
* @tc.name: GetCallerAppIdentifier001
* @tc.desc: return empty app identifier when bundle manager is null
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetCallerAppIdentifier001, TestSize.Level0)
{
    BmsDelegateImpl delegate;
    EXPECT_TRUE(delegate.GetCallerAppIdentifier(BUNDLE_NAME, USER_ID).empty());
}

/**
* @tc.name: GetCallerAppIdentifier002
* @tc.desc: return empty app identifier when querying bundle info fails
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetCallerAppIdentifier002, TestSize.Level0)
{
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(bundleMgr, nullptr);
    BmsDelegateImpl delegate;
    delegate.object_ = bundleMgr;

    EXPECT_CALL(*bundleMgr, RemoveDeathRecipient(_)).WillOnce(Return(true));
    EXPECT_CALL(*bundleMgr, GetBundleInfoV9(BUNDLE_NAME, _, _, USER_ID))
        .WillOnce(Return(ERROR_CODE));
    EXPECT_TRUE(delegate.GetCallerAppIdentifier(BUNDLE_NAME, USER_ID).empty());
}

/**
* @tc.name: GetCallerAppIdentifier003
* @tc.desc: return app identifier when querying bundle info succeeds
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, GetCallerAppIdentifier003, TestSize.Level0)
{
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(bundleMgr, nullptr);
    BmsDelegateImpl delegate;
    delegate.object_ = bundleMgr;

    EXPECT_CALL(*bundleMgr, RemoveDeathRecipient(_)).WillOnce(Return(true));
    EXPECT_CALL(*bundleMgr, GetBundleInfoV9(BUNDLE_NAME, _, _, USER_ID))
        .WillOnce(Invoke([](const std::string &, int32_t, BundleInfo &bundleInfo, int32_t) {
            bundleInfo.signatureInfo.appIdentifier = APP_IDENTIFIER;
            return ERR_OK;
        }));
    EXPECT_EQ(delegate.GetCallerAppIdentifier(BUNDLE_NAME, USER_ID), APP_IDENTIFIER);
}

/**
* @tc.name: OnRemoteDied001
* @tc.desc: clear cached BMS object when remote object died
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, OnRemoteDied001, TestSize.Level0)
{
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(bundleMgr, nullptr);
    BmsDelegateImpl delegate;
    delegate.object_ = bundleMgr;
    delegate.deathRecipient_ = new (std::nothrow) BmsDelegateImpl::ServiceDeathRecipient(&delegate);
    ASSERT_NE(delegate.deathRecipient_, nullptr);

    EXPECT_CALL(*bundleMgr, RemoveDeathRecipient(_)).WillOnce(Return(true));
    delegate.OnRemoteDied();
    EXPECT_EQ(delegate.object_, nullptr);
    EXPECT_EQ(delegate.deathRecipient_, nullptr);
}

/**
* @tc.name: OnRemoteDied002
* @tc.desc: keep cache empty when remote object died without cached BMS object
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, OnRemoteDied002, TestSize.Level0)
{
    BmsDelegateImpl delegate;
    delegate.OnRemoteDied();
    EXPECT_EQ(delegate.object_, nullptr);
    EXPECT_EQ(delegate.deathRecipient_, nullptr);
}

/**
* @tc.name: ServiceDeathRecipient001
* @tc.desc: service death recipient ignores null owner
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, ServiceDeathRecipient001, TestSize.Level0)
{
    BmsDelegateImpl::ServiceDeathRecipient recipient(nullptr);
    recipient.OnRemoteDied(nullptr);
    SUCCEED();
}

/**
* @tc.name: ServiceDeathRecipient002
* @tc.desc: service death recipient clears owner cache
* @tc.type: FUNC
*/
HWTEST_F(BmsDelegateTest, ServiceDeathRecipient002, TestSize.Level0)
{
    auto bundleMgr = sptr<BundleMgrMock>(new (std::nothrow) BundleMgrMock());
    ASSERT_NE(bundleMgr, nullptr);
    BmsDelegateImpl delegate;
    delegate.object_ = bundleMgr;

    EXPECT_CALL(*bundleMgr, RemoveDeathRecipient(_)).WillOnce(Return(true));
    BmsDelegateImpl::ServiceDeathRecipient recipient(&delegate);
    recipient.OnRemoteDied(nullptr);
    EXPECT_EQ(delegate.object_, nullptr);
    EXPECT_EQ(delegate.deathRecipient_, nullptr);
}
} // namespace

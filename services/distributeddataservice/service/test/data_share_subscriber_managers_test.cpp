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
#define LOG_TAG "DataShareSubscriberManagersTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "datashare_errno.h"
#include "data_share_service_impl.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "published_data_subscriber_manager.h"
#include "rdb_subscriber_manager.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
std::string DATA_SHARE_URI_TEST = "datashare://com.acts.ohos.data.subscribermanagertest/test";
std::string BUNDLE_NAME_TEST = "ohos.subscribermanagertest.demo";
namespace OHOS::Test {
class DataShareSubscriberManagersTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static constexpr int64_t TEST_SUB_ID = 100;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(){};
    void TearDown(){};
};

void DataShareSubscriberManagersTest::SetUpTestCase(void)
{
    HapInfoParams info = {
        .userID = USER_TEST,
        .bundleName = "ohos.subscribermanagertest.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.subscribermanagertest.demo"
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = "ohos.subscribermanagertest.demo",
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "ohos.subscribermanagertest.demo",
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.test",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    AccessTokenKit::AllocHapToken(info, policy);
    auto testTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(
        info.userID, info.bundleName, info.instIndex);
    SetSelfTokenID(testTokenId);
}

void DataShareSubscriberManagersTest::TearDownTestCase(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.subscribermanagertest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: Add
* @tc.desc: test add and delete abnormal case
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, Add, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    PredicateTemplateNode node1("p1", "select name0 as name from TBL00");
    PredicateTemplateNode node2("p2", "select name1 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    Template tpl(nodes, "select name1 as name from TBL00");
    DataShare::Key key(DATA_SHARE_URI_TEST, tpltId.subscriberId_, tpltId.bundleName_);
    auto result = TemplateManager::GetInstance().Add(key, context->currentUserId, tpl);
    EXPECT_EQ(result, DataShare::E_ERROR);
    result = TemplateManager::GetInstance().Delete(key, context->currentUserId);
    EXPECT_EQ(result, DataShare::E_ERROR);
}

/**
* @tc.name: Key
* @tc.desc: test Key operator< subscriberId function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, Key, TestSize.Level1)
{
    DataShare::Key key1(DATA_SHARE_URI_TEST, TEST_SUB_ID, BUNDLE_NAME_TEST);
    DataShare::Key key2(DATA_SHARE_URI_TEST, 0, BUNDLE_NAME_TEST);

    EXPECT_FALSE(key1 < key2);
    EXPECT_TRUE(key2 < key1);
}

/**
* @tc.name: Delete
* @tc.desc: test Delete abnormal case
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, Delete, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    DataShare::Key key(context->uri, TEST_SUB_ID, BUNDLE_NAME_TEST);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, BUNDLE_NAME_TEST, USER_TEST);
    RdbSubscriberManager::GetInstance().Delete(tokenId);
    auto result = RdbSubscriberManager::GetInstance().Delete(key, tokenId);
    EXPECT_EQ(result, E_SUBSCRIBER_NOT_EXIST);
}

/**
* @tc.name: Disable
* @tc.desc: test Disable Enable abnormal case
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, Disable, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    DataShare::Key key(context->uri, tpltId.subscriberId_, tpltId.bundleName_);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, BUNDLE_NAME_TEST, USER_TEST);
    auto result = RdbSubscriberManager::GetInstance().Disable(key, tokenId);
    EXPECT_EQ(result, E_SUBSCRIBER_NOT_EXIST);

    result = RdbSubscriberManager::GetInstance().Enable(key, context);
    EXPECT_EQ(result, E_SUBSCRIBER_NOT_EXIST);
}

/**
* @tc.name: Emit
* @tc.desc: test Emit EmitByKey GetEnableObserverCount abnormal case
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, Emit, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    RdbSubscriberManager::GetInstance().Emit(DATA_SHARE_URI_TEST, context);
    RdbSubscriberManager::GetInstance().Emit(DATA_SHARE_URI_TEST, TEST_SUB_ID, context);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    DataShare::Key key(context->uri, tpltId.subscriberId_, tpltId.bundleName_);
    RdbSubscriberManager::GetInstance().EmitByKey(key, USER_TEST, "rdbPath", 1);
    DataShare::Key keys("", 0, "");
    auto result = RdbSubscriberManager::GetInstance().GetEnableObserverCount(keys);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: IsNotifyOnEnabled
* @tc.desc: test IsNotifyOnEnabled PutInto SetObserversNotifiedOnEnabled abnormal case
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, IsNotifyOnEnabled, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    DataShare::PublishedDataKey key(context->uri, tpltId.bundleName_, tpltId.subscriberId_);
    sptr<IDataProxyPublishedDataObserver> observer;
    std::vector<PublishedDataSubscriberManager::ObserverNode> val;
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> callbacks;
    PublishedDataSubscriberManager::GetInstance().PutInto(callbacks, val, key, observer);
    std::vector<PublishedDataKey> publishedKeys;
    PublishedDataSubscriberManager::GetInstance().SetObserversNotifiedOnEnabled(publishedKeys);
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, BUNDLE_NAME_TEST, USER_TEST);
    DataShare::PublishedDataKey keys("", "", 0);
    auto result = PublishedDataSubscriberManager::GetInstance().IsNotifyOnEnabled(keys, tokenId);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: PublishedDataKey
* @tc.desc: test PublishedDataKey operator< bundleName function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareSubscriberManagersTest, PublishedDataKey, TestSize.Level1)
{
    DataShare::PublishedDataKey key1(DATA_SHARE_URI_TEST, "ohos.subscribermanagertest.demo", TEST_SUB_ID);
    DataShare::PublishedDataKey key2(DATA_SHARE_URI_TEST, "ohos.error.demo", TEST_SUB_ID);

    EXPECT_FALSE(key1 < key2);
    EXPECT_TRUE(key2 < key1);
}
} // namespace OHOS::Test
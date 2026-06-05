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
#include "data_share_service_impl.h"
#include "datashare_errno.h"
#include "data_proxy_observer.h"
#include "data_share_obs_proxy.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "proxy_data_subscriber_manager.h"
#include "published_data_subscriber_manager.h"
#include "rdb_subscriber_manager.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
std::string DATA_SHARE_URI_TEST = "datashare://com.acts.ohos.data.subscribermanagertest/test";
std::string DATA_SHARE_SUBSCRIBE_TEST_URI = "datashareproxy://com.acts.ohos.data.subscribermanagertest/test";
std::string BUNDLE_NAME_TEST = "ohos.subscribermanagertest.demo";
namespace OHOS::Test {
using OHOS::DataShare::LogLabel;
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
    auto result = TemplateManager::GetInstance().Add(key, context->visitedUserId, tpl);
    EXPECT_EQ(result, DataShare::E_ERROR);
    result = TemplateManager::GetInstance().Delete(key, context->visitedUserId);
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
    uint32_t pid = getpid();
    RdbSubscriberManager::GetInstance().Delete(tokenId, pid);
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
    RdbSubscriberManager::GetInstance().Emit(DATA_SHARE_URI_TEST, TEST_SUB_ID, BUNDLE_NAME_TEST, context);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    DataShare::Key key(context->uri, tpltId.subscriberId_, tpltId.bundleName_);
    DistributedData::StoreMetaData metaData;
    metaData.version = 1;
    metaData.tokenId = 0;
    metaData.dataDir = "rdbPath";
    RdbSubscriberManager::GetInstance().EmitByKey(key, USER_TEST, metaData);
    DataShare::Key keys("", 0, "");
    auto result = RdbSubscriberManager::GetInstance().GetEnableObserverCount(keys);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: EmitByKey
* @tc.desc: use storageManager to get a remote object and create a observer by it, use the observer to subscribe
            DATA_SHARE_SUBSCRIBE_TEST_URI , then call EmitByKey to emit by the key DATA_SHARE_SUBSCRIBE_TEST_URI
* @tc.type: FUNC
* @tc.require: None
*/
HWTEST_F(DataShareSubscriberManagersTest, EmitByKey, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_SUBSCRIBE_TEST_URI);
    context->visitedUserId = USER_TEST;
    context->callerTokenId = GetSelfTokenID();
    DataShare::Key key(context->uri, TEST_SUB_ID, BUNDLE_NAME_TEST);
    std::string dataDir = "/data/service/el1/public/database/distributeddata/kvdb";
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 1);
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    int storageManagerSAId = 5003;
    auto remoteObj = saManager->GetSystemAbility(storageManagerSAId);
    ASSERT_NE(remoteObj, nullptr);
    sptr<IDataProxyRdbObserver> observer = new (std::nothrow)RdbObserverProxy(remoteObj);
    ASSERT_NE(observer, nullptr);
    auto ret1 = RdbSubscriberManager::GetInstance().Add(key, observer, context, executors);
    EXPECT_EQ(ret1, DataShare::E_OK);

    DistributedData::StoreMetaData metaData;
    RdbSubscriberManager::GetInstance().EmitByKey(key, USER_TEST, metaData);

    auto ret2 = RdbSubscriberManager::GetInstance().Delete(key, context->callerTokenId);
    EXPECT_EQ(ret2, DataShare::E_OK);
    RdbSubscriberManager::GetInstance().EmitByKey(key, USER_TEST, metaData);
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
    PublishedDataSubscriberManager::GetInstance().PutInto(callbacks, val, key, observer, context->visitedUserId);
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

/**
* @tc.name: EmitWhenEnableAndDisable
* @tc.desc: test Enable and then Disable case, Emit only Notify when enable
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareSubscriberManagersTest, EmitWhenEnableAndDisable, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    uint32_t selfTokenId = GetSelfTokenID();
    context->callerTokenId = selfTokenId;
    DataShare::Key key(context->uri, TEST_SUB_ID, BUNDLE_NAME_TEST);
 
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(remoteObj, nullptr);
    sptr<IDataProxyRdbObserver> observer = new (std::nothrow) RdbObserverProxy(remoteObj);
    std::shared_ptr<ExecutorPool> executorPool = std::make_shared<ExecutorPool>(1, 1);
    ASSERT_NE(executorPool, nullptr);
    auto result1 = RdbSubscriberManager::GetInstance().Add(key, observer, context, executorPool);
    EXPECT_EQ(result1, DataShare::E_OK);
 
    auto result2 = RdbSubscriberManager::GetInstance().Disable(key, selfTokenId);
    EXPECT_EQ(result2, DataShare::E_OK);
    RdbSubscriberManager::GetInstance().Emit(DATA_SHARE_URI_TEST, context);
    
    auto result3 = RdbSubscriberManager::GetInstance().Enable(key, context);
    EXPECT_EQ(result3, DataShare::E_OK);
    RdbSubscriberManager::GetInstance().Emit(DATA_SHARE_URI_TEST, context);
    auto result4 = RdbSubscriberManager::GetInstance().Delete(key, selfTokenId);
    EXPECT_EQ(result4, DataShare::E_OK);
}
 
/**
* @tc.name: NotFirstSubscribe
* @tc.desc: test not first subscribe case
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareSubscriberManagersTest, NotFirstSubscribe, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    TemplateId tpltId;
    tpltId.subscriberId_ = TEST_SUB_ID;
    tpltId.bundleName_ = BUNDLE_NAME_TEST;
    DataShare::Key key(context->uri, tpltId.subscriberId_, tpltId.bundleName_);

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(remoteObj, nullptr);
    sptr<IDataProxyRdbObserver> observer = new (std::nothrow)RdbObserverProxy(remoteObj);
    std::shared_ptr<ExecutorPool> executorPool = std::make_shared<ExecutorPool>(1, 1);
    ASSERT_NE(executorPool, nullptr);
    auto result = RdbSubscriberManager::GetInstance().Add(key, observer, context, executorPool);
    EXPECT_EQ(result, DataShare::E_OK);

    result = RdbSubscriberManager::GetInstance().Add(key, observer, context, executorPool);
    EXPECT_EQ(result, DataShare::E_OK);
}

/**
* @tc.name: NotNotifyAfterEnable
* @tc.desc: test Disable and then Enable case, Notify shouldn't be called after Enable
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareSubscriberManagersTest, NotNotifyAfterEnable, TestSize.Level1)
{
    auto context = std::make_shared<Context>(DATA_SHARE_URI_TEST);
    uint32_t selfTokenId = GetSelfTokenID();
    context->callerTokenId = selfTokenId;
    DataShare::Key key(context->uri, TEST_SUB_ID, BUNDLE_NAME_TEST);

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(saManager, nullptr);
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    ASSERT_NE(remoteObj, nullptr);
    sptr<IDataProxyRdbObserver> observer = new (std::nothrow) RdbObserverProxy(remoteObj);
    std::shared_ptr<ExecutorPool> executorPool = std::make_shared<ExecutorPool>(1, 1);
    ASSERT_NE(executorPool, nullptr);
    auto result1 = RdbSubscriberManager::GetInstance().Add(key, observer, context, executorPool);
    EXPECT_EQ(result1, DataShare::E_OK);

    auto result2 = RdbSubscriberManager::GetInstance().Disable(key, selfTokenId);
    EXPECT_EQ(result2, DataShare::E_OK);

    auto result3 = RdbSubscriberManager::GetInstance().Enable(key, context);
    EXPECT_EQ(result3, DataShare::E_OK);
}

/**
* @tc.name: BuildChangeInfo001
* @tc.desc: Verify BuildChangeInfo for non-multiValues data
* @tc.type: FUNC
* @tc.require: MultiValues notification adaptation
*/
HWTEST_F(DataShareSubscriberManagersTest, BuildChangeInfo001, TestSize.Level1)
{
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo001 start");
    DataShareProxyData data;
    data.uri_ = "datashareproxy://com.test/normal";
    data.value_ = std::string("hello");
    data.isMultiValues_ = false;

    auto result = ProxyDataSubscriberManager::BuildChangeInfo(
        DataShareObserver::ChangeType::INSERT, data);

    EXPECT_EQ(result.changeType_, DataShareObserver::ChangeType::INSERT);
    EXPECT_EQ(result.uri_, "datashareproxy://com.test/normal");
    EXPECT_EQ(std::get<std::string>(result.value_), "hello");
    EXPECT_FALSE(result.isMultiValues_);
    EXPECT_TRUE(result.multiValues_.empty());
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo001 end");
}

/**
* @tc.name: BuildChangeInfo002
* @tc.desc: Verify BuildChangeInfo for multiValues with single app and single key
* @tc.type: FUNC
* @tc.require: MultiValues notification adaptation
*/
HWTEST_F(DataShareSubscriberManagersTest, BuildChangeInfo002, TestSize.Level1)
{
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo002 start");
    DataShareProxyData data;
    data.uri_ = "datashareproxy://com.test/multi";
    data.value_ = std::string("");
    data.isMultiValues_ = true;
    data.multiValues_["app1"]["key1"] = DataProxyValue("value1");

    auto result = ProxyDataSubscriberManager::BuildChangeInfo(
        DataShareObserver::ChangeType::UPDATE, data);

    EXPECT_EQ(result.changeType_, DataShareObserver::ChangeType::UPDATE);
    EXPECT_EQ(result.uri_, "datashareproxy://com.test/multi");
    EXPECT_TRUE(result.isMultiValues_);
    ASSERT_EQ(result.multiValues_.size(), 1u);
    EXPECT_EQ(std::get<std::string>(result.multiValues_[0]), "value1");
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo002 end");
}

/**
* @tc.name: BuildChangeInfo003
* @tc.desc: Verify BuildChangeInfo flattens multiValues from multiple apps into a single vector
* @tc.type: FUNC
* @tc.require: MultiValues notification adaptation
*/
HWTEST_F(DataShareSubscriberManagersTest, BuildChangeInfo003, TestSize.Level1)
{
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo003 start");
    DataShareProxyData data;
    data.uri_ = "datashareproxy://com.test/multi";
    data.value_ = std::string("");
    data.isMultiValues_ = true;
    data.multiValues_["app1"]["key1"] = DataProxyValue("v1");
    data.multiValues_["app1"]["key2"] = DataProxyValue(42);
    data.multiValues_["app2"]["key3"] = DataProxyValue(true);

    auto result = ProxyDataSubscriberManager::BuildChangeInfo(
        DataShareObserver::ChangeType::DELETE, data);

    EXPECT_EQ(result.changeType_, DataShareObserver::ChangeType::DELETE);
    EXPECT_TRUE(result.isMultiValues_);
    ASSERT_EQ(result.multiValues_.size(), 3u);
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo003 end");
}

/**
* @tc.name: BuildChangeInfo004
* @tc.desc: Verify BuildChangeInfo for multiValues with isMultiValues_=true but empty map
* @tc.type: FUNC
* @tc.require: MultiValues notification adaptation
*/
HWTEST_F(DataShareSubscriberManagersTest, BuildChangeInfo004, TestSize.Level1)
{
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo004 start");
    DataShareProxyData data;
    data.uri_ = "datashareproxy://com.test/multi_empty";
    data.value_ = std::string("");
    data.isMultiValues_ = true;

    auto result = ProxyDataSubscriberManager::BuildChangeInfo(
        DataShareObserver::ChangeType::INSERT, data);

    EXPECT_TRUE(result.isMultiValues_);
    EXPECT_TRUE(result.multiValues_.empty());
    ZLOGI("DataShareSubscriberManagersTest BuildChangeInfo004 end");
}
} // namespace OHOS::Test
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
#define LOG_TAG "DataShareServiceImplTest"

#include "data_share_service_impl.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "account_delegate_mock.h"
#include "bundle_mgr_proxy.h"
#include "bundle_utils.h"
#include "data_share_service_stub.h"
#include "device_manager_adapter.h"
#include "dump/dump_manager.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
std::string SLIENT_ACCESS_URI = "datashareproxy://com.acts.ohos.data.datasharetest/test";
std::string TBL_NAME0 = "name0";
std::string TBL_NAME1 = "name1";
std::string BUNDLE_NAME = "ohos.datasharetest.demo";
namespace OHOS::Test {
using OHOS::DataShare::LogLabel;
class DataShareServiceImplTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static constexpr int64_t TEST_SUB_ID = 100;
    static constexpr uint32_t CUREEENT_USER_ID = 123;
    static constexpr uint32_t NATIVE_USER_ID = 0;
    static void SetUpTestCase(void)
    {
        accountDelegateMock = new (std::nothrow) AccountDelegateMock();
        if (accountDelegateMock != nullptr) {
            AccountDelegate::instance_ = nullptr;
            AccountDelegate::RegisterAccountInstance(accountDelegateMock);
        }
    }
    static void TearDownTestCase(void)
    {
        if (accountDelegateMock != nullptr) {
            delete accountDelegateMock;
            accountDelegateMock = nullptr;
        }
    }
    void SetSelfTokenInfo(int32_t user);
    void SetUp();
    void TearDown();
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
};

void DataShareServiceImplTest::SetSelfTokenInfo(int32_t user)
{
    HapInfoParams info = { .userID = user,
        .bundleName = "ohos.datasharetest.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.datasharetest.demo" };
    HapPolicyParams policy = { .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = { { .permissionName = "ohos.permission.test",
            .bundleName = "ohos.datasharetest.demo",
            .grantMode = 1,
            .availableLevel = APL_NORMAL,
            .label = "label",
            .labelId = 1,
            .description = "ohos.datasharetest.demo",
            .descriptionId = 1 } },
        .permStateList = { { .permissionName = "ohos.permission.test",
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 } } } };
    AccessTokenKit::AllocHapToken(info, policy);
    auto testTokenId =
        Security::AccessToken::AccessTokenKit::GetHapTokenID(info.userID, info.bundleName, info.instIndex);
    SetSelfTokenID(testTokenId);
}
void DataShareServiceImplTest::SetUp(void)
{
    SetSelfTokenInfo(USER_TEST);
}

void DataShareServiceImplTest::TearDown(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: DataShareServiceImpl001
* @tc.desc: test InsertEx UpdateEx Query DeleteEx abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, DataShareServiceImpl001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "";
    valuesBucket.Put("", name0);
    auto [errCode, result] = dataShareServiceImpl.InsertEx(uri, "", valuesBucket);
    EXPECT_EQ((errCode != 0), true);

    DataShare::DataSharePredicates predicates;
    std::string selections = "";
    predicates.SetWhereClause(selections);
    auto [errCode1, result1] = dataShareServiceImpl.UpdateEx(uri, "", predicates, valuesBucket);
    EXPECT_EQ((errCode1 != 0), true);

    predicates.EqualTo("", "");
    std::vector<std::string> columns;
    DataShareOption option;
    auto [errVal, resQuery] = dataShareServiceImpl.Query(uri, "", predicates, columns, option);
    int resultSet = 0;
    if (resQuery != nullptr) {
        resQuery->GetRowCount(resultSet);
    }
    EXPECT_EQ(resultSet, 0);

    predicates.SetWhereClause(selections);
    auto [errCode2, result2] = dataShareServiceImpl.DeleteEx(uri, "", predicates);
    EXPECT_EQ((errCode2 != 0), true);
}

/**
* @tc.name: NotifyChange001
* @tc.desc: test NotifyChange function and abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, NotifyChange001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    auto result = dataShareServiceImpl.NotifyChange(uri, USER_TEST);
    EXPECT_EQ(result, true);

    result = dataShareServiceImpl.NotifyChange("", USER_TEST);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: AddTemplate001
* @tc.desc: test AddTemplate function and abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, AddTemplate001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    int64_t subscriberId = 0;
    PredicateTemplateNode node1("p1", "select name0 as name from TBL00");
    PredicateTemplateNode node2("p2", "select name1 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    Template tpl(nodes, "select name1 as name from TBL00");

    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tpl);
    EXPECT_EQ((result > 0), true);
    result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_EQ((result > 0), true);

    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tpl);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
    result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: GetCallerBundleName001
* @tc.desc: test GetCallerBundleName function and abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerBundleName001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    auto result = dataShareServiceImpl.GetCallerBundleName(BUNDLE_NAME);
    EXPECT_EQ(result, true);

    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    result = dataShareServiceImpl.GetCallerBundleName(BUNDLE_NAME);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: Publish001
* @tc.desc: test Publish and GetData no GetCallerBundleName scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Publish001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    std::string bundleName = "com.acts.ohos.data.datasharetest";
    data.datas_.emplace_back("datashareproxy://com.acts.ohos.data.datasharetest/test", TEST_SUB_ID, "value1");
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    std::vector<OperationResult> result = dataShareServiceImpl.Publish(data, bundleName);
    EXPECT_NE(result.size(), data.datas_.size());
    for (auto const &results : result) {
        EXPECT_NE(results.errCode_, 0);
    }

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleName, errCode);
    EXPECT_EQ(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: Publish002
* @tc.desc: test Publish uri and GetData bundleName is error
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Publish002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    std::string bundleName = "com.acts.ohos.error";
    data.datas_.emplace_back("datashareproxy://com.acts.ohos.error", TEST_SUB_ID, "value1");
    std::vector<OperationResult> result = dataShareServiceImpl.Publish(data, bundleName);
    EXPECT_EQ(result.size(), data.datas_.size());
    for (auto const &results : result) {
        EXPECT_EQ(results.errCode_, E_URI_NOT_EXIST);
    }

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleName, errCode);
    EXPECT_EQ(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: SubscribeRdbData001
* @tc.desc: Tests the operation of a subscription to a data change of the specified URI
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, SubscribeRdbData001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    PredicateTemplateNode node("p1", "select name0 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node);
    Template tpl(nodes, "select name1 as name from TBL00");
    auto result1 = dataShareServiceImpl.AddTemplate(SLIENT_ACCESS_URI, TEST_SUB_ID, tpl);
    EXPECT_TRUE(result1);
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    sptr<IDataProxyRdbObserver> observer;
    TemplateId tplId;
    tplId.subscriberId_ = TEST_SUB_ID;
    tplId.bundleName_ = BUNDLE_NAME;
    std::vector<OperationResult> result2 = dataShareServiceImpl.SubscribeRdbData(uris, tplId, observer);
    EXPECT_EQ(result2.size(), uris.size());
    for (auto const &operationResult : result2) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result3 = dataShareServiceImpl.EnableRdbSubs(uris, tplId);
    for (auto const &operationResult : result3) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket1, valuesBucket2;
    std::string name0 = "wang";
    valuesBucket1.Put(TBL_NAME0, name0);
    auto [errCode4, result4] = dataShareServiceImpl.InsertEx(uri, "", valuesBucket1);
    EXPECT_EQ((errCode4 != 0), true);

    std::vector<OperationResult> result5 = dataShareServiceImpl.UnsubscribeRdbData(uris, tplId);
    EXPECT_EQ(result5.size(), uris.size());
    for (auto const &operationResult : result5) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::string name1 = "wu";
    valuesBucket2.Put(TBL_NAME1, name1);
    auto [errCode6, result6] = dataShareServiceImpl.InsertEx(uri, "", valuesBucket2);
    EXPECT_EQ((errCode6 != 0), true);

    std::vector<OperationResult> result7 = dataShareServiceImpl.DisableRdbSubs(uris, tplId);
    for (auto const &operationResult : result7) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: SubscribePublishedData001
* @tc.desc: test SubscribePublishedData no GetCallerBundleName scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, SubscribePublishedData001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    sptr<IDataProxyPublishedDataObserver> observer;
    int64_t subscriberId = TEST_SUB_ID;

    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    std::vector<OperationResult> result = dataShareServiceImpl.SubscribePublishedData(uris, subscriberId, observer);
    EXPECT_NE(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }

    result = dataShareServiceImpl.UnsubscribePublishedData(uris, subscriberId);
    EXPECT_NE(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: SubscribePublishedData002
* @tc.desc: test SubscribePublishedData abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, SubscribePublishedData002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back("");
    sptr<IDataProxyPublishedDataObserver> observer;
    int64_t subscriberId = 0;
    std::vector<OperationResult> result = dataShareServiceImpl.SubscribePublishedData(uris, subscriberId, observer);
    EXPECT_EQ(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    result = dataShareServiceImpl.UnsubscribePublishedData(uris, subscriberId);
    EXPECT_EQ(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: EnablePubSubs001
* @tc.desc: test EnablePubSubs no GetCallerBundleName scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, EnablePubSubs001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    int64_t subscriberId = TEST_SUB_ID;

    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    dataShareServiceImpl.OnConnectDone();
    std::vector<OperationResult> result = dataShareServiceImpl.EnablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }

    result = dataShareServiceImpl.DisablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: EnablePubSubs002
* @tc.desc: test EnablePubSubs abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, EnablePubSubs002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back("");
    int64_t subscriberId = 0;
    std::vector<OperationResult> result = dataShareServiceImpl.EnablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    result = dataShareServiceImpl.DisablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: OnAppUninstall
* @tc.desc: Test the operation of the app
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnAppUninstall, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    pid_t uid = 1;
    pid_t pid = 2;
    int32_t user = USER_TEST;
    int32_t index = 0;
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, BUNDLE_NAME, 0);

    auto result = dataShareServiceImpl.OnAppUpdate(BUNDLE_NAME, user, index);
    EXPECT_EQ(result, GeneralError::E_OK);

    result = dataShareServiceImpl.OnAppExit(uid, pid, tokenId, BUNDLE_NAME);
    EXPECT_EQ(result, GeneralError::E_OK);

    result = dataShareServiceImpl.OnAppUninstall(BUNDLE_NAME, user, index);
    EXPECT_EQ(result, GeneralError::E_OK);

    DataShareServiceImpl::DataShareStatic dataShareStatic;
    result = dataShareStatic.OnAppUninstall(BUNDLE_NAME, user, index);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: OnInitialize
* @tc.desc: test OnInitialize function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnInitialize, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params;
    dataShareServiceImpl.DumpDataShareServiceInfo(fd, params);
    auto result = dataShareServiceImpl.OnInitialize();
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: GetCallerInfo001
* @tc.desc: test GetCallerInfo function when succeeded in getting tokenID
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerInfo001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest GetCallerInfo001 start");
    DataShareServiceImpl dataShareServiceImpl;
    int32_t appIndex = 1;
    auto result = dataShareServiceImpl.GetCallerInfo(BUNDLE_NAME, appIndex);
    EXPECT_EQ(result.first, true);
    ZLOGI("DataShareServiceImplTest GetCallerInfo001 end");
}

/**
* @tc.name: GetCallerInfo002
* @tc.desc: test GetCallerInfo function when failed to get tokenID
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerInfo002, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest GetCallerInfo002 start");
    DataShareServiceImpl dataShareServiceImpl;
    int32_t appIndex = 1;
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    auto result = dataShareServiceImpl.GetCallerInfo(BUNDLE_NAME, appIndex);
    EXPECT_EQ(result.first, false);
    ZLOGI("DataShareServiceImplTest GetCallerInfo002 end");
}

/**
* @tc.name: GetSilentProxyStatus001
* @tc.desc: test GetSilentProxyStatus001 function while creating helper
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareServiceImplTest, GetSilentProxyStatus001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest GetSilentProxyStatus001 start");
    SetSelfTokenInfo(NATIVE_USER_ID);
    DataShareServiceImpl dataShareServiceImpl;
    auto result = dataShareServiceImpl.GetSilentProxyStatus(SLIENT_ACCESS_URI, true);
    EXPECT_EQ(result, OHOS::DataShare::E_OK);

    result = dataShareServiceImpl.GetSilentProxyStatus(SLIENT_ACCESS_URI + "?user=100", true);
    EXPECT_EQ(result, OHOS::DataShare::E_OK);
    SetSelfTokenInfo(USER_TEST);
    ZLOGI("DataShareServiceImplTest GetSilentProxyStatus001 end");
}

/**
* @tc.name: DataProviderConfig001
* @tc.desc: test get provider info function
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareServiceImplTest, DataProviderConfig001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest DataProviderConfig001 start");
    SetSelfTokenInfo(NATIVE_USER_ID);
    auto testTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(NATIVE_USER_ID, BUNDLE_NAME, 0);
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(testing::_)).Times(1).WillOnce(testing::Return(false));
    DataProviderConfig config(SLIENT_ACCESS_URI, testTokenId);
    EXPECT_EQ(config.providerInfo_.uri, SLIENT_ACCESS_URI);
    EXPECT_EQ(config.providerInfo_.currentUserId, NATIVE_USER_ID);
    EXPECT_EQ(config.providerInfo_.visitedUserId, NATIVE_USER_ID);
    SetSelfTokenInfo(USER_TEST);
    ZLOGI("DataShareServiceImplTest DataProviderConfig001 end");
}

/**
* @tc.name: UpdateLaunchInfoMock001
* @tc.desc: mock GetInstance return nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareServiceImplTest, UpdateLaunchInfo001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest UpdateLaunchInfo001 start");

    DataShareServiceImpl dataShareServiceImpl;
    // cover branch of config is nullptr
    dataShareServiceImpl.UpdateLaunchInfo();

    std::string prefix = StoreMetaData::GetPrefix({ DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid });
    std::vector<StoreMetaData> storeMetaData;
    MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true);
    EXPECT_EQ(storeMetaData.size(), 0);
    ZLOGI("DataShareServiceImplTest UpdateLaunchInfo001 end");
}

/**
* @tc.name: BundleMgrProxyTest001
* @tc.desc: Test the CheckSilentConfig method of BundleMgrProxy
* @tc.type: FUNC
* @tc.require: 
*/
HWTEST_F(DataShareServiceImplTest, BundleMgrProxyTest001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest BundleMgrProxyTest001 start");
    int32_t user = static_cast<int32_t>(USER_TEST);
    auto bundleMgr = BundleMgrProxy::GetInstance();
    auto [err, ret] = bundleMgr->CheckSilentConfig(BUNDLE_NAME, user);
    EXPECT_EQ(err, OHOS::DataShare::E_SILENT_PROXY_DISABLE);
    EXPECT_EQ(ret, false);
    auto [err2, ret2] = bundleMgr->CheckSilentConfig("", user);
    EXPECT_NE(err2, OHOS::DataShare::E_OK);
    EXPECT_EQ(ret2, false);
    ZLOGI("DataShareServiceImplTest BundleMgrProxyTest001 end");
}

/**
* @tc.name: BundleUtilsTest001
* @tc.desc: Test the SetBundleInfoCallback and CheckSilentConfig methods of BundleUtils
* @tc.type: FUNC
* @tc.require: 
*/
HWTEST_F(DataShareServiceImplTest, BundleUtilsTest001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplTest BundleUtilsTest001 start");

    auto [err, ret] = BundleUtils::GetInstance().CheckSilentConfig("", 0);
    EXPECT_EQ(err, -1);
    EXPECT_EQ(ret, false);

    auto task = [](const std::string &bundleName, int32_t userId) {
        return std::make_pair(0, true);
    };
    BundleUtils::GetInstance().SetBundleInfoCallback(task);
    auto [err2, ret2] = BundleUtils::GetInstance().CheckSilentConfig("", 0);
    EXPECT_EQ(err2, 0);
    EXPECT_EQ(ret2, true);
    ZLOGI("DataShareServiceImplTest BundleUtilsTest001 end");
}
} // namespace OHOS::Test
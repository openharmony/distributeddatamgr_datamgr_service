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

#define LOG_TAG "DataShareServiceImplTest"

#define private public
#include "data_share_service_impl.h"
#undef private

#include <gtest/gtest.h>
#include <unistd.h>
#include "log_print.h"
#include "ipc_skeleton.h"
#include "data_share_service_stub.h"
#include "dump/dump_manager.h"
#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using DumpManager = OHOS::DistributedData::DumpManager;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
std::string SLIENT_ACCESS_URI = "datashare:///com.acts.datasharetest/entry/DB00/TBL00?Proxy=true";
std::string TBL_NAME0 = "name0";
std::string TBL_NAME1 = "name1";
std::string  BUNDLE_NAME = "ohos.datasharetest.demo";
std::string  BUNDLE_NAME_ERROR = "ohos.error.demo";
namespace OHOS::Test {
class DataShareServiceImplTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static constexpr int64_t TEST_SUB_ID = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp();
    void TearDown();
protected:
};

void DataShareServiceImplTest::SetUp(void)
{
    HapInfoParams info = {
        .userID = 100,
        .bundleName = "ohos.datasharetest.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.datasharetest.demo"
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = "ohos.datasharetest.demo",
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "ohos.datasharetest.demo",
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

void DataShareServiceImplTest::TearDown(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: Insert001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "wang";
    valuesBucket.Put(TBL_NAME0, name0);
    std::string name1 = "wu";
    valuesBucket.Put(TBL_NAME1, name1);
    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Insert002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShare::DataShareValuesBucket valuesBucket;
    std::string value = "";
    std::string name = "";
    valuesBucket.Put(name, value);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Insert003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "wang";
    valuesBucket.Put(TBL_NAME0, name0);
    std::string name1 = "wu";
    valuesBucket.Put(TBL_NAME1, name1);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Insert004
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert004, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "wang";
    valuesBucket.Put(TBL_NAME0, name0);
    std::string name1 = "wu";
    valuesBucket.Put(TBL_NAME1, name1);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: NotifyChange001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, NotifyChange001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    auto result = dataShareServiceImpl.NotifyChange(uri);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: NotifyChange002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, NotifyChange002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    auto result = dataShareServiceImpl.NotifyChange(uri);
    EXPECT_EQ(result, true);
}

/**
* @tc.name: Update001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Update001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "wang";
    valuesBucket.Put(TBL_NAME0, name0);
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_NAME1 + " = 'wu'";
    predicates.SetWhereClause(selections);
    auto result = dataShareServiceImpl.Update(uri, predicates ,valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Update002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Update002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "";
    valuesBucket.Put(TBL_NAME0, name0);
    DataShare::DataSharePredicates predicates;
    std::string selections = "";
    predicates.SetWhereClause(selections);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Update(uri, predicates ,valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Update003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Update003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "wang";
    valuesBucket.Put(TBL_NAME0, name0);
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_NAME1 + " = 'wu'";
    predicates.SetWhereClause(selections);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Update(uri, predicates ,valuesBucket);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Delete001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Delete001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_NAME1 + " = 'wu'";
    predicates.SetWhereClause(selections);

    auto result = dataShareServiceImpl.Delete(uri, predicates);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Delete002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Delete002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShare::DataSharePredicates predicates;
    std::string selections = "";
    predicates.SetWhereClause(selections);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Delete(uri, predicates);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Delete003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Delete003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_NAME1 + " = 'wu'";
    predicates.SetWhereClause(selections);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Delete(uri, predicates);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: Query001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Query001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TBL_NAME1, "wu");
    std::vector<std::string> columns;
    int errCode = 0;

    auto result = dataShareServiceImpl.Query(uri, predicates, columns, errCode);
    int resultSet = 0;
    if (result != nullptr) {
        result->GetRowCount(resultSet);
    }
    EXPECT_EQ(resultSet, 0);
}

/**
* @tc.name: Query002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Query002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("", "");
    std::vector<std::string> columns;
    int errCode = 0;

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Query(uri, predicates, columns, errCode);
    int resultSet = 0;
    if (result != nullptr) {
        result->GetRowCount(resultSet);
    }
    EXPECT_EQ(resultSet, 0);
}

/**
* @tc.name: Query003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Query003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TBL_NAME1, "wu");
    std::vector<std::string> columns;
    int errCode = 0;

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, DataShare::E_OK);

    auto result = dataShareServiceImpl.Query(uri, predicates, columns, errCode);
    int resultSet = 0;
    if (result != nullptr) {
        result->GetRowCount(resultSet);
    }
    EXPECT_EQ(resultSet, 0);
}

/**
* @tc.name: AddTemplate001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, AddTemplate001, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    int64_t subscriberId = TEST_SUB_ID;
    PredicateTemplateNode node1("p1", "select name0 as name from TBL00");
    PredicateTemplateNode node2("p2", "select name1 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    Template tpl(nodes, "select name1 as name from TBL00");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tpl);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);

    result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: AddTemplate002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, AddTemplate002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    int64_t subscriberId = 0;
    PredicateTemplateNode node1("p1", "select name0 as name from TBL00");
    PredicateTemplateNode node2("p2", "select name1 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    Template tpl(nodes, "select name1 as name from TBL00");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tpl);
    EXPECT_TRUE(result);

    result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_TRUE(result);
}

/**
* @tc.name: AddTemplate003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, AddTemplate003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    int64_t subscriberId = TEST_SUB_ID;
    PredicateTemplateNode node1("p1", "select name0 as name from TBL00");
    PredicateTemplateNode node2("p2", "select name1 as name from TBL00");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    nodes.emplace_back(node2);
    Template tpl(nodes, "select name1 as name from TBL00");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tpl);
    EXPECT_TRUE(result);

    result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_TRUE(result);
}

/**
* @tc.name: GetCallerBundleName001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerBundleName001, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::string bundleName = BUNDLE_NAME;
    auto result = dataShareServiceImpl.GetCallerBundleName(bundleName);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: GetCallerBundleName002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerBundleName002, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::string bundleName = BUNDLE_NAME_ERROR;
    auto result = dataShareServiceImpl.GetCallerBundleName(bundleName);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: GetCallerBundleName003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetCallerBundleName003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string bundleName = BUNDLE_NAME;
    auto result = dataShareServiceImpl.GetCallerBundleName(bundleName);
    EXPECT_EQ(result, true);
}

/**
* @tc.name: Publish001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Publish001, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    data.datas_.emplace_back(BUNDLE_NAME, TEST_SUB_ID, "value1");
    std::string bundleNameOfProvider = BUNDLE_NAME;
    std::vector<OperationResult> result = dataShareServiceImpl.Publish(data, bundleNameOfProvider);
    EXPECT_NE(result.size(), data.datas_.size());

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleNameOfProvider, errCode);
    EXPECT_EQ(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: Publish002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Publish002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    data.datas_.emplace_back(BUNDLE_NAME, TEST_SUB_ID, "value1");
    std::string bundleNameOfProvider = BUNDLE_NAME_ERROR;
    std::vector<OperationResult> result = dataShareServiceImpl.Publish(data, bundleNameOfProvider);
    EXPECT_EQ(result.size(), data.datas_.size());

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleNameOfProvider, errCode);
    EXPECT_EQ(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: Publish003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Publish003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    data.datas_.emplace_back(BUNDLE_NAME, TEST_SUB_ID, "value1");
    std::string bundleNameOfProvider = BUNDLE_NAME;
    std::vector<OperationResult> result = dataShareServiceImpl.Publish(data, bundleNameOfProvider);
    EXPECT_EQ(result.size(), data.datas_.size());

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleNameOfProvider, errCode);
    EXPECT_EQ(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: GetData001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, GetData001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    DataShare::Data data;
    data.datas_.emplace_back(BUNDLE_NAME, TEST_SUB_ID, "value1");
    std::string bundleNameOfProvider = "";

    int errCode = 0;
    auto getData = dataShareServiceImpl.GetData(bundleNameOfProvider, errCode);
    EXPECT_NE(errCode, 0);
    EXPECT_NE(getData.datas_.size(), data.datas_.size());
}

/**
* @tc.name: SubscribeRdbData001
* @tc.desc:
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
    std::vector<OperationResult> result2 =  dataShareServiceImpl.SubscribeRdbData(uris, tplId, observer);
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
    auto result4 = dataShareServiceImpl.Insert(uri, valuesBucket1);
    EXPECT_EQ((result4 > 0), false);

    std::vector<OperationResult> result5 = dataShareServiceImpl.UnsubscribeRdbData(uris, tplId);
    EXPECT_EQ(result5.size(), uris.size());
    for (auto const &operationResult : result5) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::string name1 = "wu";
    valuesBucket2.Put(TBL_NAME1, name1);
    auto result6 = dataShareServiceImpl.Insert(uri, valuesBucket2);
    EXPECT_EQ((result6 > 0), false);

    std::vector<OperationResult> result7 = dataShareServiceImpl.DisableRdbSubs(uris, tplId);
    for (auto const &operationResult : result7) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: SubscribePublishedData001
* @tc.desc:
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
    std::vector<OperationResult> result =  dataShareServiceImpl.SubscribePublishedData(uris, subscriberId, observer);
    EXPECT_EQ(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.UnsubscribePublishedData(uris, subscriberId);
    EXPECT_EQ(result1.size(), uris.size());
    for (auto const &operationResult : result1) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: SubscribePublishedData002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, SubscribePublishedData002, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    sptr<IDataProxyPublishedDataObserver> observer;
    int64_t subscriberId = TEST_SUB_ID;
    std::vector<OperationResult> result =  dataShareServiceImpl.SubscribePublishedData(uris, subscriberId, observer);
    EXPECT_NE(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.UnsubscribePublishedData(uris, subscriberId);
    EXPECT_NE(result1.size(), uris.size());
    for (auto const &operationResult : result1) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: SubscribePublishedData003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, SubscribePublishedData003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back("");
    sptr<IDataProxyPublishedDataObserver> observer;
    int64_t subscriberId = 0;
    std::vector<OperationResult> result =  dataShareServiceImpl.SubscribePublishedData(uris, subscriberId, observer);
    EXPECT_EQ(result.size(), uris.size());
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.UnsubscribePublishedData(uris, subscriberId);
    EXPECT_EQ(result1.size(), uris.size());
    for (auto const &operationResult : result1) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: EnablePubSubs001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, EnablePubSubs001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    int64_t subscriberId = TEST_SUB_ID;
    std::vector<OperationResult> result =  dataShareServiceImpl.EnablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.DisablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result1) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: EnablePubSubs002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, EnablePubSubs002, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back(SLIENT_ACCESS_URI);
    int64_t subscriberId = TEST_SUB_ID;
    std::vector<OperationResult> result =  dataShareServiceImpl.EnablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.DisablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result1) {
        EXPECT_EQ(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: EnablePubSubs003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, EnablePubSubs003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris;
    uris.emplace_back("");
    int64_t subscriberId = 0;
    std::vector<OperationResult> result =  dataShareServiceImpl.EnablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result) {
        EXPECT_NE(operationResult.errCode_, 0);
    }

    std::vector<OperationResult> result1 =  dataShareServiceImpl.DisablePubSubs(uris, subscriberId);
    for (auto const &operationResult : result1) {
        EXPECT_NE(operationResult.errCode_, 0);
    }
}

/**
* @tc.name: OnBind
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnBind, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    FeatureSystem::Feature::BindInfo binderInfo;
    auto result1 = dataShareServiceImpl.OnBind(binderInfo);
    dataShareServiceImpl.OnConnectDone();
    EXPECT_EQ(result1, GeneralError::E_OK);
}

/**
* @tc.name: OnAppUninstall
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnAppUninstall, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string bundleName = BUNDLE_NAME;
    int32_t user = USER_TEST;
    int32_t index = 0;
    auto result1 = dataShareServiceImpl.OnAppUninstall(bundleName, user, index);
    EXPECT_EQ(result1, GeneralError::E_OK);

    DataShareServiceImpl::DataShareStatic dataShareStatic;
    auto result2 = dataShareStatic.OnAppUninstall(bundleName, user, index);
    EXPECT_EQ(result2, GeneralError::E_OK);
}

/**
* @tc.name: OnInitialize
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnInitialize, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    int fd = 1;
    std::map<std::string, std::vector<std::string>> params;
    dataShareServiceImpl.DumpDataShareServiceInfo(fd, params);
    auto result1 = dataShareServiceImpl.OnInitialize();
    EXPECT_EQ(result1, 0);
}

/**
* @tc.name: OnAppExit
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, OnAppExit, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    pid_t uid = 1;
    pid_t pid = 2;
    int32_t user = USER_TEST;
    int32_t index = 0;
    uint32_t tokenId = AccessTokenKit::GetHapTokenID(100, BUNDLE_NAME, 0);
    std::string bundleName = BUNDLE_NAME;
    auto result1 = dataShareServiceImpl.OnAppUpdate(bundleName, user, index);
    EXPECT_EQ(result1, GeneralError::E_OK);

    auto result2 = dataShareServiceImpl.OnAppExit(uid, pid, tokenId, bundleName);
    EXPECT_EQ(result2, GeneralError::E_OK);
}

/**
* @tc.name: RegisterObserver001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, RegisterObserver001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    sptr<OHOS::IRemoteObject> remoteObj;
    auto result1 = dataShareServiceImpl.RegisterObserver(uri, remoteObj);
    EXPECT_NE(result1, GeneralError::E_OK);
    dataShareServiceImpl.NotifyObserver(uri);
    auto result2 = dataShareServiceImpl.UnregisterObserver(uri, remoteObj);
    EXPECT_NE(result2, GeneralError::E_OK);
}

/**
* @tc.name: RegisterObserver002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, RegisterObserver002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string urierr = "";
    sptr<OHOS::IRemoteObject> remoteObj;
    auto result1 = dataShareServiceImpl.RegisterObserver(urierr, remoteObj);
    EXPECT_NE(result1, GeneralError::E_OK);
    dataShareServiceImpl.NotifyObserver(urierr);
    auto result2 = dataShareServiceImpl.UnregisterObserver(urierr, remoteObj);
    EXPECT_NE(result2, GeneralError::E_OK);
}

/**
* @tc.name: RegisterObserver003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, RegisterObserver003, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    sptr<OHOS::IRemoteObject> remoteObj;
    auto result1 = dataShareServiceImpl.RegisterObserver(uri, remoteObj);
    EXPECT_NE(result1, GeneralError::E_OK);
    dataShareServiceImpl.NotifyObserver(uri);
    auto result2 = dataShareServiceImpl.UnregisterObserver(uri, remoteObj);
    EXPECT_NE(result2, GeneralError::E_OK);
}
} // namespace OHOS::Test
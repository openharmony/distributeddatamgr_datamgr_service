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
std::string TBL_STU_NAME = "name";
std::string TBL_STU_AGE = "age";
std::string  BUNDLE_NAME = "ohos.datasharetest.demo";
namespace OHOS::Test {
class DataShareServiceImplTest : public testing::Test {
public:
    static constexpr int64_t TEST_SUB_ID = 123;
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
    std::string value = "lisi";
    valuesBucket.Put(TBL_STU_NAME, value);
    int age = 25;
    valuesBucket.Put(TBL_STU_AGE, age);
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
    std::string value = "lisi";
    valuesBucket.Put(TBL_STU_NAME, value);
    int age = 25;
    valuesBucket.Put(TBL_STU_AGE, age);

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
    std::string value = "lisi";
    valuesBucket.Put(TBL_STU_NAME, value);
    int age = 25;
    valuesBucket.Put(TBL_STU_AGE, age);

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
    EXPECT_EQ(result, false);
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
    int value = 50;
    valuesBucket.Put(TBL_STU_AGE, value);
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_STU_NAME + " = 'lisi'";
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
    int value = 0;
    valuesBucket.Put(TBL_STU_AGE, value);
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
    int value = 50;
    valuesBucket.Put(TBL_STU_AGE, value);
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_STU_NAME + " = 'lisi'";
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
    std::string selections = TBL_STU_NAME + " = 'lisi'";
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
    std::string selections = TBL_STU_NAME + " = 'lisi'";
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
    predicates.EqualTo(TBL_STU_NAME, "lisi");
    std::vector<std::string> columns;
    int errCode = -1;

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
    int errCode = -1;

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
    predicates.EqualTo(TBL_STU_NAME, "lisi");
    std::vector<std::string> columns;
    int errCode = -1;

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
    PredicateTemplateNode node1("key", "selectSql");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    Template tplt(nodes, "scheduler");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tplt);
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
    PredicateTemplateNode node1("key", "selectSql");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    Template tplt(nodes, "scheduler");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tplt);
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
    PredicateTemplateNode node1("key", "selectSql");
    std::vector<PredicateTemplateNode> nodes;
    nodes.emplace_back(node1);
    Template tplt(nodes, "scheduler");
    auto result = dataShareServiceImpl.AddTemplate(uri, subscriberId, tplt);
    EXPECT_TRUE(result);
}

/**
* @tc.name: DelTemplate001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, DelTemplate001, TestSize.Level1)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.datasharetest.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    int64_t subscriberId = TEST_SUB_ID;
    auto result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_EQ(result, DataShareServiceImpl::ERROR);
}

/**
* @tc.name: DelTemplate002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, DelTemplate002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    int64_t subscriberId = 0;
    auto result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
    EXPECT_TRUE(result);
}

/**
* @tc.name: DelTemplate003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, DelTemplate003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = SLIENT_ACCESS_URI;
    int64_t subscriberId = TEST_SUB_ID;
    auto result = dataShareServiceImpl.DelTemplate(uri, subscriberId);
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
    std::string bundleName = "";
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
} // namespace OHOS::Test
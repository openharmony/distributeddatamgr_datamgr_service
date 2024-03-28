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

#include <gtest/gtest.h>
#include "log_print.h"
#include "ipc_skeleton.h"
#include "data_share_service_impl.h"
#include "data_share_service_stub.h"
#include "dump/dump_manager.h"

using namespace testing::ext;
using DumpManager = OHOS::DistributedData::DumpManager;
using namespace OHOS::DataShare;

constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
std::string DATA_SHARE_URI = "datashare:///com.acts.datasharetest";
std::string SLIENT_ACCESS_URI = "datashare:///com.acts.datasharetest/entry/DB00/TBL00?Proxy=true";
std::string SLIENT_REGISTER_URI = "datashare:///com.acts.datasharetest/entry/DB00/TBL02?Proxy=true";
std::string TBL_STU_NAME = "name";
std::string TBL_STU_AGE = "age";

namespace OHOS::Test {
class DataShareServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};

protected:
};

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

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, E_OK);

    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_NE(result, -1);
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
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    std::string value = "lisi";
    valuesBucket.Put(TBL_STU_NAME, value);
    int age = 25;
    valuesBucket.Put(TBL_STU_AGE, age);
    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, ERROR);
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

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, E_OK);

    auto result = dataShareServiceImpl.Update(uri, predicates ,valuesBucket);
    EXPECT_NE(result, -1);
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
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataShareValuesBucket valuesBucket;
    int value = 50;
    valuesBucket.Put(TBL_STU_AGE, value);
    DataShare::DataSharePredicates predicates;
    std::string selections = TBL_STU_NAME + " = 'lisi'";
    predicates.SetWhereClause(selections);
    auto result = dataShareServiceImpl.Update(uri, predicates ,valuesBucket);
    EXPECT_EQ(result, ERROR);
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
    DataShare::DataSharePredicates deletePredicates;
    std::string selections = TBL_STU_NAME + " = 'lisi'";
    deletePredicates.SetWhereClause(selections);

    bool enable = true;
    auto resultA = dataShareServiceImpl.EnableSilentProxy(uri, enable);
    EXPECT_EQ(resultA, E_OK);

    auto result = dataShareServiceImpl.Delete(uri, predicates ,valuesBucket);
    EXPECT_NE(result, -1);
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
    std::string uri = SLIENT_ACCESS_URI;
    DataShare::DataSharePredicates deletePredicates;
    std::string selections = TBL_STU_NAME + " = 'lisi'";
    deletePredicates.SetWhereClause(selections);

    auto result = dataShareServiceImpl.Delete(uri, predicates ,valuesBucket);
    EXPECT_EQ(result, ERROR);
}

} // namespace OHOS::Test
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

#define LOG_TAG "CloudInfoTest"
#include <gtest/gtest.h>

#include "serializable/serializable.h"
#include "cloud/cloud_info.h"
#include "cloud/schema_meta.h"
#include "nlohmann/json.hpp"
#include "utils/crypto.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
class CloudInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: GetSchemaPrefix
* @tc.desc: Get schema prefix.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaPrefix, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaPrefix("ohos.test.demo");
    ASSERT_EQ(result, "CLOUD_SCHEMA###0###ohos.test.demo");

    result = cloudInfo.GetSchemaPrefix("");
    ASSERT_EQ(result, "CLOUD_SCHEMA###0");
}

/**
* @tc.name: IsValid
* @tc.desc: Determine if it is limited.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsValid, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsValid();
    ASSERT_FALSE(result);

    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;

    Serializable::json node1;
    cloudInfo1.Marshal(node1);
    result = cloudInfo1.IsValid();
    ASSERT_TRUE(result);
}

/**
* @tc.name: Exist
* @tc.desc: Determine if the package exists.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, Exist, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.Exist("", 1);
    ASSERT_FALSE(result);

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = false;

    cloudInfo.user = 111;
    cloudInfo.id = "test_cloud_id";
    cloudInfo.totalSpace = 0;
    cloudInfo.remainSpace = 100;
    cloudInfo.enableCloud = true;
    cloudInfo.apps["test_cloud_bundleName"] = std::move(appInfo);

    Serializable::json node1;
    cloudInfo.Marshal(node1);
    result = cloudInfo.Exist("test_cloud_bundleName", 100);
    ASSERT_TRUE(result);
}

/**
* @tc.name: Exist
* @tc.desc: Is it on.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsOn, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsOn("ohos.test.demo", 1);
    ASSERT_FALSE(result);

    CloudInfo::AppInfo appInfo;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;

    cloudInfo.user = 111;
    cloudInfo.id = "test_cloud_id";
    cloudInfo.totalSpace = 0;
    cloudInfo.remainSpace = 100;
    cloudInfo.enableCloud = true;
    cloudInfo.apps["test_cloud_bundleName"] = std::move(appInfo);

    Serializable::json node1;
    cloudInfo.Marshal(node1);
    result = cloudInfo.IsOn("test_cloud_bundleName", 100);
    ASSERT_TRUE(result);
}

/**
* @tc.name: GetPrefix
* @tc.desc: Get prefix.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetPrefix, TestSize.Level0)
{
    const std::initializer_list<std::string> fields;
    auto result = CloudInfo::GetPrefix(fields);
    ASSERT_EQ(result, "CLOUD_INFO###");
}

/**
* @tc.name: CloudInfoTest
* @tc.desc: Marshal and Unmarshal of CloudInfo.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, CloudInfoTest, TestSize.Level0)
{
    CloudInfo cloudInfo1;
    cloudInfo1.user = 111;
    cloudInfo1.id = "test1_id";
    cloudInfo1.totalSpace = 0;
    cloudInfo1.remainSpace = 0;
    cloudInfo1.enableCloud = false;

    Serializable::json node1;
    cloudInfo1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), to_string(node1));

    CloudInfo cloudInfo2;
    cloudInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfo1), Serializable::Marshall(cloudInfo1));
}

/**
* @tc.name: AppInfoTest
* @tc.desc: Marshal and Unmarshal of AppInfo.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, AppInfoTest, TestSize.Level0)
{
    CloudInfo::AppInfo cloudInfoAppInfo1;
    cloudInfoAppInfo1.bundleName = "ohos.test.demo";
    cloudInfoAppInfo1.appId = "test1_id";
    cloudInfoAppInfo1.version = 0;
    cloudInfoAppInfo1.instanceId = 0;
    cloudInfoAppInfo1.cloudSwitch = false;

    Serializable::json node1;
    cloudInfoAppInfo1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), to_string(node1));

    CloudInfo::AppInfo cloudInfoAppInfo2;
    cloudInfoAppInfo2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(cloudInfoAppInfo1), Serializable::Marshall(cloudInfoAppInfo2));
}

/**
* @tc.name: TableTest
* @tc.desc: Marshal and Unmarshal of Table.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, TableTest, TestSize.Level0)
{
    Field field1;
    field1.colName = "test1_colName";
    field1.alias = "test1_alias";
    field1.type = 1;
    field1.primary = true;
    field1.nullable = false;

    Table table1;
    table1.name = "test1_name";
    table1.sharedTableName = "test1_sharedTableName";
    table1.alias = "test1_alias";
    table1.fields.push_back(field1);
    Serializable::json node1;
    table1.Marshal(node1);
    EXPECT_EQ(Serializable::Marshall(table1), to_string(node1));

    Table table2;
    table2.Unmarshal(node1);
    EXPECT_EQ(Serializable::Marshall(table1), Serializable::Marshall(table2));
}

/**
* @tc.name: IsAllSwitchOffTest
* @tc.desc: Determine if all apps have their cloud synchronization disabled.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, IsAllSwitchOffTest, TestSize.Level0)
{
    CloudInfo cloudInfo;
    auto result = cloudInfo.IsAllSwitchOff();
    ASSERT_TRUE(result);

    CloudInfo cloudInfo1;
    CloudInfo::AppInfo appInfo;
    std::map<std::string, CloudInfo::AppInfo> apps;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;
    apps = { { "test_cloud", appInfo } };
    cloudInfo1.apps = apps;
    result = cloudInfo1.IsAllSwitchOff();
    ASSERT_FALSE(result);
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest1, TestSize.Level0)
{
    std::string bundleName = "test_cloud_bundleName";
    int32_t instanceId = 123;
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(bundleName, instanceId);
    ASSERT_EQ(result, "CLOUD_SCHEMA###0###test_cloud_bundleName###123");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest2, TestSize.Level0)
{
    int32_t user = 123;
    std::string bundleName = "test_cloud_bundleName";
    int32_t instanceId = 456;
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(user, bundleName, instanceId);
    ASSERT_EQ(result, "CLOUD_SCHEMA###123###test_cloud_bundleName###456");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest3, TestSize.Level0)
{
    StoreMetaData meta;
    meta.bundleName = "test_cloud_bundleName";
    meta.user = "123";
    CloudInfo cloudInfo;
    auto result = cloudInfo.GetSchemaKey(meta);
    ASSERT_EQ(result, "CLOUD_SCHEMA###123###test_cloud_bundleName###0");
}

/**
* @tc.name: GetSchemaKey
* @tc.desc: Get schema key.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Anvette
*/
HWTEST_F(CloudInfoTest, GetSchemaKeyTest4, TestSize.Level0)
{
    CloudInfo cloudInfo;
    CloudInfo::AppInfo appInfo;
    std::map<std::string, CloudInfo::AppInfo> apps;
    appInfo.bundleName = "test_cloud_bundleName";
    appInfo.appId = "test_cloud_id";
    appInfo.version = 0;
    appInfo.instanceId = 100;
    appInfo.cloudSwitch = true;
    apps = { { "test_cloud", appInfo } };
    cloudInfo.apps = apps;
    std::map<std::string, std::string> info;
    info = { {"test_cloud_bundleName", "CLOUD_SCHEMA###0###test_cloud###100"} };
    auto result = cloudInfo.GetSchemaKey();
    ASSERT_EQ(info, result);
}

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
#define LOG_TAG "DataShareProfileConfigTest"

#include "data_share_profile_config.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "data_share_db_config.h"
#include "data_share_service_impl.h"
#include "datashare_errno.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
std::string DATA_SHARE_URI = "datashare:///com.acts.datasharetest";
std::string DATA_SHARE_SA_URI = "datashareproxy://com.acts.datasharetest/SAID=12321/test";
constexpr uint32_t CALLERTOKENID = 100;
class DataShareProfileConfigTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: GetDbConfig001
* @tc.desc: test data_share_db_config is GetDbConfig abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, GetDbConfig001, TestSize.Level1)
{
    ZLOGI("DataShareProfileConfigTest GetDbConfig001 start");
    DataShareDbConfig dbConfig;
    DataShareDbConfig::DbConfig config {"", "", "", "", "", 0, false};
    auto result = dbConfig.GetDbConfig(config);
    EXPECT_EQ(std::get<0>(result), NativeRdb::E_DB_NOT_EXIST);

    config.hasExtension = true;
    result = dbConfig.GetDbConfig(config);
    EXPECT_EQ(std::get<0>(result), NativeRdb::E_DB_NOT_EXIST);

    config.uri = DATA_SHARE_URI;
    config.bundleName = "bundleName";
    config.storeName = "storeName";
    config.userId = USER_TEST;
    result = dbConfig.GetDbConfig(config);
    EXPECT_NE(std::get<0>(result), DataShare::E_OK);
    ZLOGI("DataShareProfileConfigTest GetDbConfig001 end");
}

/**
 * @tc.name: GetDbConfig002
 * @tc.desc: Verify GetDbConfig behavior with non-existent SA Id and missing metaData
 * @tc.type: FUNC
 * @tc.precon: DataShareDbConfig is initialized
 * @tc.step:
 *   1. Set config with non-existent SA Id, expect E_DB_NOT_EXIST
 *   2. Set config with SA Id distributeddata, metaData not exist, expect E_DB_NOT_EXIST
 * @tc.expect:
 *   1. Return E_DB_NOT_EXIST
 *   2. Return E_DB_NOT_EXIST
 */
HWTEST_F(DataShareProfileConfigTest, GetDbConfig002, TestSize.Level1)
{
    ZLOGI("DataShareProfileConfigTest GetDbConfig002 start");
    DataShareDbConfig dbConfig;
    // SA Id is not exist, just return E_DB_NOT_EXIST
    DataShareDbConfig::DbConfig config {DATA_SHARE_SA_URI, "extUri", "bundleName", "storeName", "", 0, false};
    auto result = dbConfig.GetDbConfig(config);
    EXPECT_EQ(std::get<0>(result), NativeRdb::E_DB_NOT_EXIST);

    // SA Id is distributeddata, CheckSystemAbility return not null, but metaData is not exist, return E_DB_NOT_EXIST
    config.uri = "datashareproxy://com.acts.datasharetest/SAID=1301/test";
    result = dbConfig.GetDbConfig(config);
    EXPECT_EQ(std::get<0>(result), NativeRdb::E_DB_NOT_EXIST);
    ZLOGI("DataShareProfileConfigTest GetDbConfig002 end");
}

/**
* @tc.name: Config
* @tc.desc: test config Marshal and Unmarshal function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, Config, TestSize.Level1)
{
    Config config;
    config.uri = "uri";
    config.crossUserMode = 1;
    config.writePermission = "writePermission";
    config.readPermission = "readPermission";
    DistributedData::Serializable::json node;
    config.Marshal(node);
    EXPECT_EQ(node["uri"], "uri");
    EXPECT_EQ(node["crossUserMode"], 1);
    EXPECT_EQ(node["writePermission"], "writePermission");
    EXPECT_EQ(node["readPermission"], "readPermission");

    Config configs;
    configs.Unmarshal(node);
    EXPECT_EQ(configs.uri, "uri");
    EXPECT_EQ(configs.crossUserMode, 1);
    EXPECT_EQ(configs.writePermission, "writePermission");
    EXPECT_EQ(configs.readPermission, "readPermission");
}

/**
* @tc.name: ProfileInfo001
* @tc.desc: test ProfileInfo Marshal and Unmarshal function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, ProfileInfo001, TestSize.Level1)
{
    ProfileInfo info;
    Config config;
    config.uri = "uri";
    config.crossUserMode = 1;
    config.writePermission = "writePermission";
    config.readPermission = "readPermission";
    info.tableConfig = { config };
    info.isSilentProxyEnable = 1;
    info.storeName = "storeName";
    info.tableName = "tableName";

    DistributedData::Serializable::json node;
    info.Marshal(node);
    EXPECT_EQ(node["isSilentProxyEnable"], true);
    EXPECT_EQ(node["path"], "storeName/tableName");
    EXPECT_EQ(node["scope"], "module");
    EXPECT_EQ(node["type"], "rdb");

    ProfileInfo profileInfo;
    profileInfo.Unmarshal(node);
    EXPECT_EQ(profileInfo.isSilentProxyEnable, true);
    EXPECT_EQ(profileInfo.storeName, "storeName");
    EXPECT_EQ(profileInfo.tableName, "tableName");
    EXPECT_EQ(profileInfo.scope, "module");
    EXPECT_EQ(profileInfo.type, "rdb");
}

/**
* @tc.name: ProfileInfo002
* @tc.desc: test ProfileInfo Unmarshal abnormal scenario
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, ProfileInfo002, TestSize.Level1)
{
    ProfileInfo info;
    Config config;
    config.uri = "uri";
    config.crossUserMode = 1;
    config.writePermission = "writePermission";
    config.readPermission = "readPermission";
    info.tableConfig = { config };
    info.isSilentProxyEnable = 1;
    info.storeName = "";
    info.tableName = "";
    DistributedData::Serializable::json node1;
    info.Marshal(node1);
    EXPECT_EQ(node1["path"], "/");
    ProfileInfo profileInfo;
    profileInfo.Unmarshal(node1);
    EXPECT_EQ(profileInfo.storeName, "");
    EXPECT_EQ(profileInfo.tableName, "");

    DistributedData::Serializable::json node2;
    info.storeName = "storeName";
    info.tableName = "";
    info.Marshal(node2);
    EXPECT_EQ(node2["path"], "storeName/");
    profileInfo.Unmarshal(node2);
    EXPECT_EQ(profileInfo.storeName, "");
    EXPECT_EQ(profileInfo.tableName, "");

    DistributedData::Serializable::json node3;
    info.storeName = "";
    info.tableName = "tableName";
    info.Marshal(node3);
    EXPECT_EQ(node3["path"], "/tableName");
    profileInfo.Unmarshal(node3);
    EXPECT_EQ(profileInfo.storeName, "");
    EXPECT_EQ(profileInfo.tableName, "");
}

/**
* @tc.name: ReadProfile
* @tc.desc: test ReadProfile error scenario
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, ReadProfile, TestSize.Level1)
{
    DataShareProfileConfig profileConfig;
    auto result = profileConfig.ReadProfile("");
    EXPECT_EQ(result, "");
    result = profileConfig.ReadProfile("resPath");
    EXPECT_EQ(result, "");
}

/**
* @tc.name: GetAccessCrossMode
* @tc.desc: test GetAccessCrossMode function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, GetAccessCrossMode, TestSize.Level1)
{
    DataShareProfileConfig profileConfig;
    ProfileInfo info;
    Config config;
    config.uri = "storeUri";
    config.crossUserMode = 1;
    config.writePermission = "writePermission";
    config.readPermission = "readPermission";
    info.tableConfig = { config };
    info.isSilentProxyEnable = 1;
    info.storeName = "storeName";
    info.tableName = "tableName";

    auto result = profileConfig.GetAccessCrossMode(info, "tableUri", "storeUri");
    EXPECT_EQ(result, AccessCrossMode::USER_SHARED);
}

/**
* @tc.name: GetFromDataProperties
* @tc.desc: test GetFromDataProperties scenario
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, GetFromDataProperties, TestSize.Level1)
{
    DataProviderConfig providerConfig(DATA_SHARE_URI, CALLERTOKENID);
    providerConfig.GetProviderInfo();

    ProfileInfo info;
    info.isSilentProxyEnable = 1;
    info.storeName = "storeName";
    info.tableName = "tableName";
    info.scope = "moduletest";
    auto result = providerConfig.GetFromDataProperties(info, "moduleName");
    EXPECT_EQ(result, DataShare::E_OK);
}

/**
* @tc.name: GetFromUriPath
* @tc.desc: test GetFromUriPath error scenario
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, GetFromUriPath, TestSize.Level1)
{
    DataProviderConfig providerConfig(DATA_SHARE_URI, CALLERTOKENID);
    providerConfig.GetProviderInfo();

    ProfileInfo info;
    info.isSilentProxyEnable = 1;
    info.storeName = "";
    info.tableName = "";
    info.scope = "moduletest";
    auto result = providerConfig.GetFromDataProperties(info, "moduleName");
    EXPECT_EQ(result, DataShare::E_OK);

    result = providerConfig.GetFromUriPath();
    EXPECT_EQ(result, false);
}

/**
* @tc.name: GetFromExtension
* @tc.desc: test GetFromExtension error scenario
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProfileConfigTest, GetFromExtension, TestSize.Level1)
{
    DataProviderConfig providerConfig(DATA_SHARE_URI, CALLERTOKENID);
    providerConfig.GetProviderInfo();

    ProfileInfo info;
    Config config;
    config.uri = DATA_SHARE_URI;
    config.crossUserMode = 1;
    config.writePermission = "writePermission";
    config.readPermission = "readPermission";
    info.tableConfig = { config };
    info.isSilentProxyEnable = 1;
    info.storeName = "storeName";
    info.tableName = "tableName";
    auto result = providerConfig.GetFromDataProperties(info, "moduleName");
    EXPECT_EQ(result, DataShare::E_OK);

    result = providerConfig.GetFromExtension();
    EXPECT_EQ(result, DataShare::E_URI_NOT_EXIST);
}
} // namespace OHOS::Test
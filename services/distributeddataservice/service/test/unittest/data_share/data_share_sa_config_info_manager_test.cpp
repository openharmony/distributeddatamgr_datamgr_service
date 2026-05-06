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

#define LOG_TAG "DataShareSAConfigInfoMgrTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "common_utils.h"
#include "data_share_sa_config_info_manager.h"
#include "datashare_errno.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DataShareSAConfigInfoManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};
const std::string testUri = "datashareproxy://processName/SAID=12321/test";
const std::string noDataProxyUri = "datashare://processName/SAID=12321/test";
const std::string testReadPermission = "ohos.permission.GET_BUNDLE_INFO";
const std::string testWritePermission = "ohos.permission.WRITE_CONTACTS";
const std::string testStoreName = "storeName";
const std::string testTableName = "tableName";
const std::string testKey = "testKey";
const int32_t testId = 12321;
std::string longStr = std::string(256, 'a');

/**
 * @tc.name: GetDataShareSAConfigInfo001
 * @tc.desc: Verify the behavior of GetDataShareSAConfigInfo when not a system app or using an invalid SA Id
 * @tc.type: FUNC
 * @tc.precon: DataShareSAConfigInfoManager is initialized and ready to process config info
 * @tc.step:
 *   1. Set from system app to false and test with an invalid SA Id, expect E_NOT_SYSTEM_APP
 *   2. Set from system app to true and test with an invalid SA Id, expect E_ERROR
 * @tc.expect:
 *   1. Return E_NOT_SYSTEM_APP when not a system app
 *   2. Return E_ERROR when using an invalid SA Id
 */
HWTEST_F(DataShareSAConfigInfoManagerTest, GetDataShareSAConfigInfo001, TestSize.Level1)
{
    ZLOGI("DataShareSAConfigInfoManagerTest GetDataShareSAConfigInfo001 start");
    // set from system app false
    DataShareThreadLocal::SetFromSystemApp(false);
    auto configInfoMgr = DataShareSAConfigInfoManager::GetInstance();
    DataShareSAConfigInfo outInfo;
    // no configInfo cache and LoadConfiInfo failed by using invalid SA Id
    int32_t ret = configInfoMgr->GetDataShareSAConfigInfo(testKey, testId, outInfo);
    EXPECT_EQ(ret, E_NOT_SYSTEM_APP);

    // set from system app true
    DataShareThreadLocal::SetFromSystemApp(true);
    ret = configInfoMgr->GetDataShareSAConfigInfo(testKey, testId, outInfo);
    EXPECT_EQ(ret, E_ERROR);
    ZLOGI("DataShareSAConfigInfoManagerTest GetDataShareSAConfigInfo001 end");
}

/**
 * @tc.name: GetDataShareSAConfigInfo002
 * @tc.desc: Verify the behavior of GetDataShareSAConfigInfo with config info cache and valid SA Id
 * @tc.type: FUNC
 * @tc.precon: DataShareSAConfigInfoManager is initialized and ready to process config info
 * @tc.step:
 *   1. Set from system app to true and test with an invalid SA Id, expect E_ERROR
 *   2. Set config info cache and test with a valid SA Id, expect E_OK and correct config info
 * @tc.expect:
 *   1. Return E_ERROR when using an invalid SA Id
 *   2. Return E_OK and correct config info when using a valid SA Id and config info cache
 */
HWTEST_F(DataShareSAConfigInfoManagerTest, GetDataShareSAConfigInfo002, TestSize.Level1)
{
    ZLOGI("DataShareSAConfigInfoManagerTest GetDataShareSAConfigInfo002 start");
    DataShareThreadLocal::SetFromSystemApp(true);
    auto configInfoMgr = DataShareSAConfigInfoManager::GetInstance();
    DataShareSAConfigInfo outInfo;
    // no configInfo cache and LoadConfiInfo failed by using invalid SA Id
    int32_t ret = configInfoMgr->GetDataShareSAConfigInfo(testKey, testId, outInfo);
    EXPECT_EQ(ret, E_ERROR);

    // set configInfo cache
    SAConfigProxyData configProxyData;
    configProxyData.uri = testKey;
    std::vector<SAConfigProxyData> proxyData;
    proxyData.push_back(configProxyData);
    DataShareSAConfigInfo cacheInfo;
    cacheInfo.proxyData = proxyData;
    std::string cacheKey = testKey + std::to_string(testId);
    configInfoMgr->configCache_.Insert(cacheKey, cacheInfo);
    EXPECT_NE(configInfoMgr->configCache_.Size(), 0);
    ret = configInfoMgr->GetDataShareSAConfigInfo(testKey, testId, outInfo);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(outInfo.proxyData[0].uri, testKey);
    configInfoMgr->configCache_.Erase(cacheKey);
    ZLOGI("DataShareSAConfigInfoManagerTest GetDataShareSAConfigInfo002 end");
}

/**
 * @tc.name: DataShareSAConfigInfo001
 * @tc.desc: Verify the behavior of Unmarshal for a normal JSON string representing DataShareSAConfigInfo
 * @tc.type: FUNC
 * @tc.precon: DataShareSAConfigInfo is initialized and ready to unmarshall JSON strings
 * @tc.step:
 *   1. Test with a normal JSON string, expect correct unmarshalled SAConfigProxyData
 * @tc.expect:
 *   1. Return true and correct SAConfigProxyData for a normal JSON string
 */
HWTEST_F(DataShareSAConfigInfoManagerTest, DataShareSAConfigInfo001, TestSize.Level1)
{
    ZLOGI("DataShareSAConfigInfoManagerTest DataShareSAConfigInfo001 start");
    // normal jsonStr
    SAConfigProxyData normalData;
    normalData.uri = testUri;
    normalData.requiredReadPermission = testReadPermission;
    normalData.requiredWritePermission = testWritePermission;
    normalData.profile.storeName = testStoreName;
    normalData.profile.tableName = testTableName;
    DataShareSAConfigInfo normalInfo;
    normalInfo.proxyData.push_back(normalData);
    DistributedData::Serializable::json node;
    normalInfo.Marshal(node);

    DataShareSAConfigInfo normalInfoTest;
    bool ret = normalInfoTest.Unmarshal(node);
    EXPECT_TRUE(ret);
    EXPECT_EQ(normalInfoTest.proxyData[0].uri, testUri);
    EXPECT_EQ(normalInfoTest.proxyData[0].requiredReadPermission, testReadPermission);
    EXPECT_EQ(normalInfoTest.proxyData[0].requiredWritePermission, testWritePermission);
    EXPECT_EQ(normalInfoTest.proxyData[0].profile.storeName, testStoreName);
    ZLOGI("DataShareSAConfigInfoManagerTest DataShareSAConfigInfo001 end");
}

/**
 * @tc.name: SAConfigProxyData001
 * @tc.desc: Verify the behavior of Marshal/Unmarshal for normal SAConfigProxyData
 * @tc.type: FUNC
 * @tc.precon: SAConfigProxyData is initialized and ready to marshal/unmarshal JSON
 * @tc.step:
 *   1. Create a normal SAConfigProxyData object with valid uri, permissions, and profile
 *   2. Marshal the object to JSON node
 *   3. Unmarshal the JSON node back to a new SAConfigProxyData object
 * @tc.expect:
 *   1. Marshal succeeds and produces valid JSON node
 *   2. Unmarshal returns true
 *   3. All fields match the original data after round-trip
 */
HWTEST_F(DataShareSAConfigInfoManagerTest, SAConfigProxyData001, TestSize.Level1)
{
    ZLOGI("DataShareSAConfigInfoManagerTest SAConfigProxyData001 start");
    // normal jsonStr
    SAConfigProxyData normalData;
    normalData.uri = testUri;
    normalData.requiredReadPermission = testReadPermission;
    normalData.requiredWritePermission = testWritePermission;
    normalData.profile.storeName = testStoreName;
    normalData.profile.tableName = testTableName;
    DistributedData::Serializable::json node;
    normalData.Marshal(node);

    SAConfigProxyData normalDataTest;
    bool ret = normalDataTest.Unmarshal(node);
    EXPECT_TRUE(ret);
    EXPECT_EQ(normalDataTest.uri, testUri);
    EXPECT_EQ(normalDataTest.requiredReadPermission, testReadPermission);
    EXPECT_EQ(normalDataTest.requiredWritePermission, testWritePermission);
    EXPECT_EQ(normalDataTest.profile.storeName, testStoreName);
    ZLOGI("DataShareSAConfigInfoManagerTest SAConfigProxyData001 end");
}

/**
 * @tc.name: SAConfigProxyData002
 * @tc.desc: Verify Unmarshal fails for various invalid SAConfigProxyData inputs
 * @tc.type: FUNC
 * @tc.precon: SAConfigProxyData is initialized and ready to unmarshal JSON strings
 * @tc.step:
 *   1. Test with missing datashareproxy schema in URI, expect unmarshalling to fail
 *   2. Test with URI exceeding max length, expect unmarshalling to fail
 *   3. Test with readPermission exceeding max length, expect unmarshalling to fail
 *   4. Test with writePermission exceeding max length, expect unmarshalling to fail
 * @tc.expect:
 *   1. Return false for invalid URI schema
 *   2. Return false for invalid URI length
 *   3. Return false for invalid readPermission length
 *   4. Return false for invalid writePermission length
 */
HWTEST_F(DataShareSAConfigInfoManagerTest, SAConfigProxyData002, TestSize.Level1)
{
    ZLOGI("DataShareSAConfigInfoManagerTest SAConfigProxyData002 start");
    // normal jsonStr
    SAConfigProxyData normalData;
    normalData.requiredReadPermission = testReadPermission;
    normalData.requiredWritePermission = testWritePermission;
    normalData.profile.storeName = testStoreName;
    normalData.profile.tableName = testTableName;

    // no datashareproxy schema uri
    normalData.uri = noDataProxyUri;
    DistributedData::Serializable::json noProxyUriNode;
    normalData.Marshal(noProxyUriNode);
    SAConfigProxyData noProxyUriDataTest;
    bool ret = noProxyUriDataTest.Unmarshal(noProxyUriNode);
    EXPECT_FALSE(ret);

    // invalid uri length
    normalData.uri = testUri + longStr;
    DistributedData::Serializable::json invalidUriNode;
    normalData.Marshal(invalidUriNode);
    SAConfigProxyData invalidUriDataTest;
    ret = invalidUriDataTest.Unmarshal(invalidUriNode);
    EXPECT_FALSE(ret);
    
    // invalid readPermission length
    normalData.uri = testUri;
    normalData.requiredReadPermission = testReadPermission + longStr;
    DistributedData::Serializable::json invalidReadPermissionNode;
    normalData.Marshal(invalidReadPermissionNode);
    SAConfigProxyData invalidReadPermissionDataTest;
    ret = invalidReadPermissionDataTest.Unmarshal(invalidReadPermissionNode);
    EXPECT_FALSE(ret);

    // invalid writePermission length
    normalData.requiredReadPermission = testReadPermission;
    normalData.requiredWritePermission = testWritePermission + longStr;
    DistributedData::Serializable::json invalidWritePermissionNode;
    normalData.Marshal(invalidWritePermissionNode);
    SAConfigProxyData invalidWritePermissionDataTest;
    ret = invalidWritePermissionDataTest.Unmarshal(invalidWritePermissionNode);
    EXPECT_FALSE(ret);
    ZLOGI("DataShareSAConfigInfoManagerTest SAConfigProxyData002 end");
}
} // namespace OHOS::Test
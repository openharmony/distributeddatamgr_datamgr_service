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

#define LOG_TAG "DataShareProxyDataMgrTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "data_share_profile_config.h"
#include "dataproxy_handle_common.h"
#include "datashare_errno.h"
#include "datashare_observer.h"
#include "executor_pool.h"
#include "log_print.h"
#include "proxy_data_manager.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DataShareProxyDataMgrTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: CheckAndCorrectProxyData001
* @tc.desc: Verify CheckAndCorrectProxyData behavior with invalid and valid maxValueLength
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and DataProxyConfig is initialized
* @tc.step:
*   1. Set maxValueLength to invalid value (-1), expect CheckAndCorrectProxyData return false
*   2. Set maxValueLength to MAX_LENGTH_4K, expect CheckAndCorrectProxyData return true
* @tc.expect:
*   1. Return false when maxValueLength is invalid
*   2. Return true when maxValueLength is valid
*/
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData001, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest GetDataShareSAConfigInfo001 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = "normal value";
    proxyData.allowList_ = {"app1"};
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    // invalid maxValueLength
    config.maxValueLength_ = static_cast<DataProxyMaxValueLength>(-1);
    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_FALSE(result);

    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData001 end");
}

/**
* @tc.name: CheckAndCorrectProxyData002
* @tc.desc: Verify CheckAndCorrectProxyData behavior with string value at boundary and exceeding limit
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and DataProxyConfig is initialized with MAX_LENGTH_4K
* @tc.step:
    *   1. Set string value to exactly 4096 bytes (MAX_LENGTH_4K), expect return true
*   2. Set string value to 4097 bytes (MAX_LENGTH_4K + 1), expect return false
* @tc.expect:
*   1. Return true when string size equals MAX_LENGTH_4K
*   2. Return false when string size exceeds MAX_LENGTH_4K
*/
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData002, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData002 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = std::string(4096, 'a');
    proxyData.allowList_ = {"app1"};

    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_TRUE(result);

    proxyData.value_ = std::string(4097, 'a');
    result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData002 end");
}

/**
* @tc.name: CheckAndCorrectProxyData003
* @tc.desc: Verify CheckAndCorrectProxyData behavior with int type value
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and DataProxyConfig is initialized
* @tc.step:
*   1. Set value to int type (12345), expect return true
* @tc.expect:
*   1. Return true because int type does not require length check
*/
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData003, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData003 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = 12345;
    proxyData.allowList_ = {"app1"};
    
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    
    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData003 end");
}

/**
* @tc.name: CheckAndCorrectProxyData004
* @tc.desc: Verify CheckAndCorrectProxyData behavior with URI exceeding limit
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and DataProxyConfig is initialized
* @tc.step:
*   1. Set URI to size URI_MAX_SIZE + 1, expect return false
* @tc.expect:
*   1. Return false when URI size exceeds URI_MAX_SIZE
*/
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData004, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData004 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/" + std::string(URI_MAX_SIZE + 1, 'a');
    proxyData.value_ = "normal value";
    proxyData.allowList_ = {"app1"};
    
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    
    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData004 end");
}

/**
* @tc.name: CheckAndCorrectProxyData005
* @tc.desc: Verify CheckAndCorrectProxyData behavior with appIdentifier exceeding limit
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and DataProxyConfig is initialized
* @tc.step:
*   1. Set allowList with 3 entries, second entry exceeds APPIDENTIFIER_MAX_SIZE
*   2. Call CheckAndCorrectProxyData, expect return true
*   3. Verify that the oversized appIdentifier is removed from allowList
* @tc.expect:
*   1. Return true
*   2. allowList size reduced to 2
*   3. Oversized appIdentifier is removed, valid ones remain
*/
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData005, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData005 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = "normal value";
    proxyData.allowList_ = {
        "app1",
        std::string(APPIDENTIFIER_MAX_SIZE + 1, 'a'),
        "app2"
    };
    
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    
    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_TRUE(result);
    EXPECT_EQ(proxyData.allowList_.size(), 2);
    EXPECT_EQ(proxyData.allowList_[0], "app1");
    EXPECT_EQ(proxyData.allowList_[1], "app2");
    ZLOGI("DataShareProxyDataMgrTest CheckAndCorrectProxyData005 end");
}

/**
* @tc.name: UpdateProxyDataList001
* @tc.desc: Verify UpdateProxyDataList behavior with null delegate and empty database
* @tc.type: FUNC
* @tc.precon: PublishedProxyData is ready and BundleInfo is initialized
* @tc.step:
*   1. Call UpdateProxyDataList with nullptr delegate
*   2. Verify return INNER_ERROR
*   3. Initialize KvDBDelegate with test path and executor pool
*   4. Call UpdateProxyDataList with valid delegate but empty database
*   5. Verify return INNER_ERROR
* @tc.expect:
*   1. Return INNER_ERROR when delegate is nullptr
*   2. Return INNER_ERROR when database query returns empty result
*/
HWTEST_F(DataShareProxyDataMgrTest, UpdateProxyDataList001, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest UpdateProxyDataList001 start");
    std::string uri = "datashareproxy://com.test/test";
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.test";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 1001;

    // test delegate == nullptr
    int32_t result = PublishedProxyData::UpdateProxyDataList(nullptr, uri, callerBundleInfo);
    EXPECT_EQ(result, INNER_ERROR);
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 1);
    std::string path = "path/to/your/db";
    auto delegate = KvDBDelegate::GetInstance(path, executors);
    ASSERT_NE(delegate, nullptr);

    int32_t errCode = PublishedProxyData::UpdateProxyDataList(delegate, uri, callerBundleInfo);
    EXPECT_EQ(errCode, INNER_ERROR);
    ZLOGI("DataShareProxyDataMgrTest UpdateProxyDataList001 end");
}

/**
* @tc.name: VerifyPermission001
* @tc.desc: Verify VerifyPermission behavior when caller is publisher (tokenId match)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with tokenId = 1001
*   2. Create BundleInfo with same tokenId = 1001
*   3. Call VerifyPermission, expect return true
* @tc.expect:
*   1. Return true when caller tokenId matches data tokenId (publisher access)
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission001, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission001 start");
    // Publisher access: tokenId match
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {};  // Empty allowList, but publisher can still access

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.test.publisher";
    callerBundleInfo.appIdentifier = "1234567890123456789";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 1001;  // Same as data.tokenId

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission001 end");
}

/**
* @tc.name: VerifyPermission002
* @tc.desc: Verify VerifyPermission behavior with allowList=["all"] (lowercase)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["all"]
*   2. Create BundleInfo with different tokenId (non-publisher)
*   3. Call VerifyPermission, expect return true
* @tc.expect:
*   1. Return true when allowList contains "all" wildcard
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission002, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission002 start");
    // allowList=["all"] - global public config
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"all"};  // Lowercase "all" wildcard

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.other.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";  // Different app
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;  // Different tokenId (non-publisher)

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission002 end");
}

/**
* @tc.name: VerifyPermission003
* @tc.desc: Verify VerifyPermission behavior with allowList=["all", "xxx"] (mixed)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["all", "specific_id"]
*   2. Create BundleInfo with random appIdentifier (not in list)
*   3. Call VerifyPermission, expect return true (because "all" is present)
* @tc.expect:
*   1. Return true when allowList contains "all" even with other specific identifiers
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission003, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission003 start");
    // allowList=["all", "xxx"] - mixed allowList
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"all", "1111111111111111111", "2222222222222222222"};

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.random.app";
    callerBundleInfo.appIdentifier = "8888888888888888888";  // Not in allowList
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 3003;  // Different tokenId

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission003 end");
}

/**
* @tc.name: VerifyPermission004
* @tc.desc: Verify VerifyPermission behavior with allowList=["ALL"] (uppercase - case sensitive)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["ALL"] (uppercase)
*   2. Create BundleInfo with different tokenId and appIdentifier
*   3. Call VerifyPermission, expect return false (uppercase "ALL" is not wildcard)
* @tc.expect:
*   1. Return false because "ALL" (uppercase) is treated as regular appIdentifier
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission004, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission004 start");
    // allowList=["ALL"] - uppercase, should NOT work as wildcard
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"ALL"};  // Uppercase - not a wildcard

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.other.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission004 end");
}

/**
* @tc.name: VerifyPermission005
* @tc.desc: Verify VerifyPermission behavior with allowList=["All"] (mixed case - case sensitive)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["All"] (mixed case)
*   2. Create BundleInfo with different tokenId and appIdentifier
*   3. Call VerifyPermission, expect return false (mixed case "All" is not wildcard)
* @tc.expect:
*   1. Return false because "All" (mixed case) is treated as regular appIdentifier
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission005, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission005 start");
    // allowList=["All"] - mixed case, should NOT work as wildcard
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"All"};  // Mixed case - not a wildcard

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.other.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission005 end");
}

/**
* @tc.name: VerifyPermission006
* @tc.desc: Verify VerifyPermission behavior with empty allowList (private config)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=[] (empty)
*   2. Create BundleInfo with different tokenId (non-publisher)
*   3. Call VerifyPermission, expect return false
* @tc.expect:
*   1. Return false when allowList is empty and caller is not publisher
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission006, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission006 start");
    // allowList=[] - private config, publisher only
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/private";
    data.proxyData.value = "private_value";
    data.proxyData.allowList = {};  // Empty - only publisher can access

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.other.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;  // Different tokenId (non-publisher)

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission006 end");
}

/**
* @tc.name: VerifyPermission007
* @tc.desc: Verify VerifyPermission behavior with specific appIdentifier in allowList (match)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["specific_id"]
*   2. Create BundleInfo with matching appIdentifier
*   3. Call VerifyPermission, expect return true
* @tc.expect:
*   1. Return true when caller appIdentifier matches entry in allowList
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission007, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission007 start");
    // allowList with specific appIdentifier - match case
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"1234567890123456789", "2345678901234567890"};

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.authorized.subscriber";
    callerBundleInfo.appIdentifier = "1234567890123456789";  // Matches first entry
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;  // Different tokenId (non-publisher)

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission007 end");
}

/**
* @tc.name: VerifyPermission008
* @tc.desc: Verify VerifyPermission behavior with specific appIdentifier in allowList (no match)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["specific_id"]
*   2. Create BundleInfo with different appIdentifier (not in list)
*   3. Call VerifyPermission, expect return false
* @tc.expect:
*   1. Return false when caller appIdentifier does not match any entry in allowList
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission008, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission008 start");
    // allowList with specific appIdentifier - no match case
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"1111111111111111111", "2222222222222222222"};

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.unauthorized.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";  // Not in allowList
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;  // Different tokenId

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_FALSE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission008 end");
}

/**
* @tc.name: VerifyPermission009
* @tc.desc: Verify VerifyPermission behavior with publisher access and allowList=["all"]
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with tokenId=1001 and allowList=["all"]
*   2. Create BundleInfo with matching tokenId (publisher)
*   3. Call VerifyPermission, expect return true (publisher always has access)
* @tc.expect:
*   1. Return true when caller is publisher (tokenId match), regardless of allowList
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission009, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission009 start");
    // Publisher access with allowList=["all"] - should always succeed
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"all"};

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.test.publisher";
    callerBundleInfo.appIdentifier = "1234567890123456789";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 1001;  // Same as data.tokenId (publisher)

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission009 end");
}

/**
* @tc.name: VerifyPermission010
* @tc.desc: Verify VerifyPermission behavior with allowList=["all", "ALL"] (lowercase + uppercase)
* @tc.type: FUNC
* @tc.precon: PublishedProxyData and BundleInfo are initialized
* @tc.step:
*   1. Create ProxyDataNode with allowList=["all", "ALL"] (both cases)
*   2. Create BundleInfo with different tokenId and appIdentifier
*   3. Call VerifyPermission, expect return true (lowercase "all" works)
* @tc.expect:
*   1. Return true because lowercase "all" is present, even with uppercase "ALL"
*/
HWTEST_F(DataShareProxyDataMgrTest, VerifyPermission010, TestSize.Level1)
{
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission010 start");
    // allowList=["all", "ALL"] - mixed case, lowercase "all" should work
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.uri = "datashareproxy://com.test.publisher/config";
    data.proxyData.value = "test_value";
    data.proxyData.allowList = {"all", "ALL"};

    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = "com.other.subscriber";
    callerBundleInfo.appIdentifier = "9999999999999999999";
    callerBundleInfo.userId = 100;
    callerBundleInfo.tokenId = 2002;

    bool result = PublishedProxyData::VerifyPermission(callerBundleInfo, data);
    EXPECT_TRUE(result);
    ZLOGI("DataShareProxyDataMgrTest VerifyPermission010 end");
}

// ============================================================================
// Multi-Value Mode Unit Tests
// ============================================================================

/**
 * @tc.name: CheckValueAndTotalLimits001
 * @tc.desc: Verify single value within limit and total within limit returns SUCCESS
 * @tc.type: FUNC
 * @tc.step:
 *   1. String of 100 chars, currentTotal=0, maxValueLength=4096 -> SUCCESS
 *   2. String of 4096 chars, currentTotal=0, maxValueLength=4096 -> SUCCESS (exact boundary)
 *   3. String of 96 chars, currentTotal=4000, maxValueLength=4096 -> SUCCESS (total=4096)
 */
HWTEST_F(DataShareProxyDataMgrTest, CheckValueAndTotalLimits001, TestSize.Level1)
{
    ZLOGI("CheckValueAndTotalLimits001 start");
    DataProxyValue val100 = std::string(100, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val100, 0, 4096), OHOS::DataShare::SUCCESS);

    DataProxyValue val4096 = std::string(4096, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val4096, 0, 4096), OHOS::DataShare::SUCCESS);

    DataProxyValue val96 = std::string(96, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val96, 4000, 4096), OHOS::DataShare::SUCCESS);
    ZLOGI("CheckValueAndTotalLimits001 end");
}

/**
 * @tc.name: CheckValueAndTotalLimits002
 * @tc.desc: Verify OVER_LIMIT when value or total exceeds limits
 * @tc.type: FUNC
 * @tc.step:
 *   1. String of 4097 chars -> OVER_LIMIT (exceeds MAX_SINGLE_MULTIVALUE_LENGTH)
 *   2. String of 100 chars with currentTotal=4000 -> OVER_LIMIT (4100 > 4096)
 *   3. String of 4096 chars with currentTotal=1 -> OVER_LIMIT (4097 > 4096)
 */
HWTEST_F(DataShareProxyDataMgrTest, CheckValueAndTotalLimits002, TestSize.Level1)
{
    ZLOGI("CheckValueAndTotalLimits002 start");
    DataProxyValue val4097 = std::string(4097, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val4097, 0, 4096), OVER_LIMIT);

    DataProxyValue val100 = std::string(100, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val100, 4000, 4096), OVER_LIMIT);

    DataProxyValue val4096 = std::string(4096, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val4096, 1, 4096), OVER_LIMIT);
    ZLOGI("CheckValueAndTotalLimits002 end");
}

/**
 * @tc.name: CheckValueAndTotalLimits003
 * @tc.desc: Verify limits with MAX_LENGTH_100K configuration
 * @tc.type: FUNC
 * @tc.step:
 *   1. String 4096 chars, total=98000, max=102400 -> SUCCESS (102096 <= 102400)
 *   2. String 4096 chars, total=99000, max=102400 -> OVER_LIMIT (103096 > 102400)
 */
HWTEST_F(DataShareProxyDataMgrTest, CheckValueAndTotalLimits003, TestSize.Level1)
{
    ZLOGI("CheckValueAndTotalLimits003 start");
    DataProxyValue val4096 = std::string(4096, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val4096, 98000, 102400), OHOS::DataShare::SUCCESS);

    DataProxyValue val4096b = std::string(4096, 'a');
    EXPECT_EQ(PublishedProxyData::CheckValueAndTotalLimits(val4096b, 99000, 102400), OVER_LIMIT);
    ZLOGI("CheckValueAndTotalLimits003 end");
}

/**
 * @tc.name: CanInsertMultiValues001
 * @tc.desc: Verify publisher and trusted app can insert
 * @tc.type: FUNC
 * @tc.step:
 *   1. Publisher (tokenId match) with empty trustProviders -> true
 *   2. Non-publisher with appIdentifier in trustProviders -> true
 *   3. Non-publisher with trustProviders=["all"] -> true
 */
HWTEST_F(DataShareProxyDataMgrTest, CanInsertMultiValues001, TestSize.Level1)
{
    ZLOGI("CanInsertMultiValues001 start");
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.trustProviders = {};

    BundleInfo publisher;
    publisher.tokenId = 1001;
    publisher.appIdentifier = "app1";
    EXPECT_TRUE(PublishedProxyData::CanInsertMultiValues(publisher, data));

    data.proxyData.trustProviders = {"app2", "app3"};
    BundleInfo trustedApp;
    trustedApp.tokenId = 2002;
    trustedApp.appIdentifier = "app2";
    EXPECT_TRUE(PublishedProxyData::CanInsertMultiValues(trustedApp, data));

    data.proxyData.trustProviders = {"all"};
    BundleInfo anyApp;
    anyApp.tokenId = 3003;
    anyApp.appIdentifier = "random_app";
    EXPECT_TRUE(PublishedProxyData::CanInsertMultiValues(anyApp, data));
    ZLOGI("CanInsertMultiValues001 end");
}

/**
 * @tc.name: CanInsertMultiValues002
 * @tc.desc: Verify non-trusted and non-publisher cannot insert
 * @tc.type: FUNC
 * @tc.step:
 *   1. Non-publisher not in trustProviders -> false
 *   2. Non-publisher with empty trustProviders -> false
 */
HWTEST_F(DataShareProxyDataMgrTest, CanInsertMultiValues002, TestSize.Level1)
{
    ZLOGI("CanInsertMultiValues002 start");
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.trustProviders = {"app2"};

    BundleInfo untrustedApp;
    untrustedApp.tokenId = 2002;
    untrustedApp.appIdentifier = "app3";
    EXPECT_FALSE(PublishedProxyData::CanInsertMultiValues(untrustedApp, data));

    data.proxyData.trustProviders = {};
    BundleInfo nonPublisher;
    nonPublisher.tokenId = 2002;
    nonPublisher.appIdentifier = "app1";
    EXPECT_FALSE(PublishedProxyData::CanInsertMultiValues(nonPublisher, data));
    ZLOGI("CanInsertMultiValues002 end");
}

/**
 * @tc.name: CanModifyMultiValue001
 * @tc.desc: Verify publisher and key owner can modify
 * @tc.type: FUNC
 * @tc.step:
 *   1. Publisher (tokenId match) can modify any key -> true
 *   2. Key owner (appIdentifier match) can modify own key -> true
 *   3. Non-owner non-publisher cannot modify -> false
 *   4. Key not found -> false
 */
HWTEST_F(DataShareProxyDataMgrTest, CanModifyMultiValue001, TestSize.Level1)
{
    ZLOGI("CanModifyMultiValue001 start");
    ProxyDataNode data;
    data.tokenId = 1001;
    data.proxyData.multiValues["app1"]["key1"] = std::string("v");

    BundleInfo publisher;
    publisher.tokenId = 1001;
    publisher.appIdentifier = "app_other";
    EXPECT_TRUE(PublishedProxyData::CanModifyMultiValue(data, "key1", publisher));

    BundleInfo owner;
    owner.tokenId = 2002;
    owner.appIdentifier = "app1";
    EXPECT_TRUE(PublishedProxyData::CanModifyMultiValue(data, "key1", owner));

    BundleInfo stranger;
    stranger.tokenId = 2002;
    stranger.appIdentifier = "app2";
    EXPECT_FALSE(PublishedProxyData::CanModifyMultiValue(data, "key1", stranger));

    EXPECT_FALSE(PublishedProxyData::CanModifyMultiValue(data, "key_nonexistent", publisher));
    ZLOGI("CanModifyMultiValue001 end");
}

/**
 * @tc.name: IsConfigCompatible001
 * @tc.desc: Verify mode compatibility matrix for all ProxyDataUpsertMode combinations
 * @tc.type: FUNC
 * @tc.step:
 *   1. NORMAL_PUBLISH on non-multi -> true, on multi -> false
 *   2. PUBLISH_MULTIVALUES on non-multi -> true, on multi -> true
 *   3. PUT_MULTIVALUES on non-multi -> false, on multi -> true
 *   4. REMOVE_MULTIVALUES on non-multi -> false, on multi -> true
 */
HWTEST_F(DataShareProxyDataMgrTest, IsConfigCompatible001, TestSize.Level1)
{
    ZLOGI("IsConfigCompatible001 start");
    EXPECT_TRUE(PublishedProxyData::IsConfigCompatible(NORMAL_PUBLISH, false));
    EXPECT_FALSE(PublishedProxyData::IsConfigCompatible(NORMAL_PUBLISH, true));

    EXPECT_TRUE(PublishedProxyData::IsConfigCompatible(PUBLISH_MULTIVALUES, false));
    EXPECT_TRUE(PublishedProxyData::IsConfigCompatible(PUBLISH_MULTIVALUES, true));

    EXPECT_FALSE(PublishedProxyData::IsConfigCompatible(PUT_MULTIVALUES, false));
    EXPECT_TRUE(PublishedProxyData::IsConfigCompatible(PUT_MULTIVALUES, true));

    EXPECT_FALSE(PublishedProxyData::IsConfigCompatible(REMOVE_MULTIVALUES, false));
    EXPECT_TRUE(PublishedProxyData::IsConfigCompatible(REMOVE_MULTIVALUES, true));
    ZLOGI("IsConfigCompatible001 end");
}

/**
 * @tc.name: BuildInitMultiValues001
 * @tc.desc: Verify BuildInitMultiValues with empty multiValues sets flags correctly
 * @tc.type: FUNC
 * @tc.step:
 *   1. Create DataShareProxyData with empty multiValues_, maxValueLength=4K
 *   2. Call BuildInitMultiValues("com.test.publisher")
 *   3. Verify SUCCESS, isMultiValues_=true, value_="", multiValues_ still empty
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildInitMultiValues001, TestSize.Level1)
{
    ZLOGI("BuildInitMultiValues001 start");
    DataShareProxyData data;
    data.multiValues_.clear();
    data.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    int32_t ret = PublishedProxyData::BuildInitMultiValues(data, "com.test.publisher");
    EXPECT_EQ(ret, OHOS::DataShare::SUCCESS);
    EXPECT_TRUE(data.isMultiValues_);
    EXPECT_EQ(std::get<std::string>(data.value_), "");
    EXPECT_TRUE(data.multiValues_.empty());
    ZLOGI("BuildInitMultiValues001 end");
}

/**
 * @tc.name: BuildInitMultiValues002
 * @tc.desc: Verify placeholder merge and size validation
 * @tc.type: FUNC
 * @tc.step:
 *   1. Set multiValues under "appIdentifier" placeholder with 2 keys totaling < 4096
 *   2. Call BuildInitMultiValues, expect SUCCESS
 *   3. Verify data merged under real appIdentifier, placeholder gone
 *   4. Set oversized values (3000 + 2000 > 4096), expect OVER_LIMIT
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildInitMultiValues002, TestSize.Level1)
{
    ZLOGI("BuildInitMultiValues002 start");
    DataShareProxyData data;
    data.multiValues_["appIdentifier"]["1"] = std::string("v1");
    data.multiValues_["appIdentifier"]["2"] = std::string("v2");
    data.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    int32_t ret = PublishedProxyData::BuildInitMultiValues(data, "com.test.publisher");
    EXPECT_EQ(ret, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(data.multiValues_.size(), 1u);
    EXPECT_EQ(data.multiValues_.count("com.test.publisher"), 1u);
    EXPECT_EQ(data.multiValues_["com.test.publisher"].size(), 2u);

    DataShareProxyData overData;
    overData.multiValues_["appIdentifier"]["1"] = std::string(3000, 'a');
    overData.multiValues_["appIdentifier"]["2"] = std::string(2000, 'b');
    overData.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    EXPECT_EQ(PublishedProxyData::BuildInitMultiValues(overData, "com.test.publisher"), OVER_LIMIT);
    ZLOGI("BuildInitMultiValues002 end");
}

/**
 * @tc.name: BuildInitMultiValues003
 * @tc.desc: Verify total size exactly at boundary succeeds
 * @tc.type: FUNC
 * @tc.step:
 *   1. Set multiValues totaling exactly 4096 bytes (2000 + 2096)
 *   2. Call BuildInitMultiValues, expect SUCCESS
 *   3. Verify merged data is under real appIdentifier
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildInitMultiValues003, TestSize.Level1)
{
    ZLOGI("BuildInitMultiValues003 start");
    DataShareProxyData data;
    data.multiValues_["appIdentifier"]["1"] = std::string(2000, 'a');
    data.multiValues_["appIdentifier"]["2"] = std::string(2096, 'b');
    data.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    int32_t ret = PublishedProxyData::BuildInitMultiValues(data, "com.test.publisher");
    EXPECT_EQ(ret, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(data.multiValues_["com.test.publisher"].size(), 2u);
    EXPECT_EQ(std::get<std::string>(data.multiValues_["com.test.publisher"]["1"]).size(), 2000u);
    ZLOGI("BuildInitMultiValues003 end");
}

/**
 * @tc.name: BuildMultiValuesAfterPut001
 * @tc.desc: Verify insert new key, update existing key, and new appIdentifier creation
 * @tc.type: FUNC
 * @tc.step:
 *   1. Insert new key "k2" into existing app1 -> both k1 and k2 present
 *   2. Update existing key "k1" -> value overwritten
 *   3. Insert into new appIdentifier "app2" -> both apps present
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildMultiValuesAfterPut001, TestSize.Level1)
{
    ZLOGI("BuildMultiValuesAfterPut001 start");
    ProxyDataNode existing;
    existing.proxyData.uri = "datashareproxy://com.test/test";
    existing.proxyData.isMultiValues = true;
    existing.proxyData.maxValueLength = DataProxyMaxValueLength::MAX_LENGTH_4K;
    existing.proxyData.multiValues["app1"]["k1"] = std::string("old");

    BundleInfo caller1;
    caller1.appIdentifier = "app1";
    auto r1 = PublishedProxyData::BuildMultiValuesAfterPut(
        existing, "k2", DataProxyValue(std::string("new")), caller1);
    EXPECT_EQ(r1.multiValues_["app1"].size(), 2u);
    EXPECT_EQ(std::get<std::string>(r1.multiValues_["app1"]["k2"]), "new");

    auto r2 = PublishedProxyData::BuildMultiValuesAfterPut(
        existing, "k1", DataProxyValue(std::string("updated")), caller1);
    EXPECT_EQ(std::get<std::string>(r2.multiValues_["app1"]["k1"]), "updated");

    BundleInfo caller2;
    caller2.appIdentifier = "app2";
    auto r3 = PublishedProxyData::BuildMultiValuesAfterPut(
        existing, "k2", DataProxyValue(std::string("v2")), caller2);
    EXPECT_EQ(r3.multiValues_.size(), 2u);
    EXPECT_EQ(std::get<std::string>(r3.multiValues_["app2"]["k2"]), "v2");
    ZLOGI("BuildMultiValuesAfterPut001 end");
}

/**
 * @tc.name: BuildMultiValuesAfterRemove001
 * @tc.desc: Verify remove key, cross-app isolation, and last-key cleanup
 * @tc.type: FUNC
 * @tc.step:
 *   1. Remove "k1" from app1 with 2 keys -> k2 remains
 *   2. Remove from app1 does not affect app2's data
 *   3. Remove last key from app1 -> app1's map becomes empty
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildMultiValuesAfterRemove001, TestSize.Level1)
{
    ZLOGI("BuildMultiValuesAfterRemove001 start");
    ProxyDataNode existing;
    existing.proxyData.uri = "datashareproxy://com.test/test";
    existing.proxyData.isMultiValues = true;
    existing.proxyData.maxValueLength = DataProxyMaxValueLength::MAX_LENGTH_4K;
    existing.proxyData.multiValues["app1"]["k1"] = std::string("v1");
    existing.proxyData.multiValues["app1"]["k2"] = std::string("v2");

    BundleInfo caller1;
    caller1.appIdentifier = "app1";
    auto r1 = PublishedProxyData::BuildMultiValuesAfterRemove(existing, "k1", caller1);
    EXPECT_EQ(r1.multiValues_["app1"].size(), 1u);
    EXPECT_EQ(r1.multiValues_["app1"].count("k1"), 0u);

    existing.proxyData.multiValues["app2"]["k3"] = std::string("v3");
    auto r2 = PublishedProxyData::BuildMultiValuesAfterRemove(existing, "k1", caller1);
    EXPECT_EQ(r2.multiValues_["app2"].size(), 1u);
    EXPECT_EQ(std::get<std::string>(r2.multiValues_["app2"]["k3"]), "v3");

    ProxyDataNode singleKey;
    singleKey.proxyData.multiValues["app1"]["k1"] = std::string("v1");
    auto r3 = PublishedProxyData::BuildMultiValuesAfterRemove(singleKey, "k1", caller1);
    EXPECT_TRUE(r3.multiValues_["app1"].empty());
    ZLOGI("BuildMultiValuesAfterRemove001 end");
}

/**
 * @tc.name: BuildMultiValuesAfterRemove002
 * @tc.desc: Verify metadata preservation after remove
 * @tc.type: FUNC
 * @tc.step:
 *   1. Set full metadata (uri, allowList, trustProviders, maxValueLength=100K)
 *   2. Call BuildMultiValuesAfterRemove
 *   3. Verify all metadata copied, isMultiValues_=true, value_=""
 */
HWTEST_F(DataShareProxyDataMgrTest, BuildMultiValuesAfterRemove002, TestSize.Level1)
{
    ZLOGI("BuildMultiValuesAfterRemove002 start");
    ProxyDataNode existing;
    existing.proxyData.uri = "datashareproxy://com.test/test";
    existing.proxyData.allowList = {"app2", "app3"};
    existing.proxyData.trustProviders = {"app2"};
    existing.proxyData.isMultiValues = true;
    existing.proxyData.maxValueLength = DataProxyMaxValueLength::MAX_LENGTH_100K;
    existing.proxyData.multiValues["app1"]["k1"] = std::string("v1");

    BundleInfo caller;
    caller.appIdentifier = "app1";
    auto result = PublishedProxyData::BuildMultiValuesAfterRemove(existing, "k1", caller);

    EXPECT_EQ(result.uri_, "datashareproxy://com.test/test");
    EXPECT_EQ(result.allowList_.size(), 2u);
    EXPECT_EQ(result.trustProviders_.size(), 1u);
    EXPECT_TRUE(result.isMultiValues_);
    EXPECT_EQ(result.maxValueLength_, DataProxyMaxValueLength::MAX_LENGTH_100K);
    EXPECT_EQ(std::get<std::string>(result.value_), "");
    ZLOGI("BuildMultiValuesAfterRemove002 end");
}

/**
 * @tc.name: CheckAndCorrectProxyData006
 * @tc.desc: Verify CheckAndCorrectProxyData with multi-value modes and MAX_LENGTH_100K
 * @tc.type: FUNC
 * @tc.step:
 *   1. PUBLISH_MULTIVALUES mode with valid data -> true
 *   2. PUT_MULTIVALUES mode with valid data -> true
 *   3. MAX_LENGTH_100K with string=102400 -> true, string=102401 -> false
 */
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData006, TestSize.Level1)
{
    ZLOGI("CheckAndCorrectProxyData006 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = "";
    proxyData.allowList_ = {"app1"};
    proxyData.isMultiValues_ = true;

    DataProxyConfig config4k;
    config4k.type_ = DataProxyType::SHARED_CONFIG;
    config4k.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;
    EXPECT_TRUE(PublishedProxyData::CheckAndCorrectProxyData(proxyData, config4k, PUBLISH_MULTIVALUES));
    EXPECT_TRUE(PublishedProxyData::CheckAndCorrectProxyData(proxyData, config4k, PUT_MULTIVALUES));

    DataProxyConfig config100k;
    config100k.type_ = DataProxyType::SHARED_CONFIG;
    config100k.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_100K;
    proxyData.value_ = std::string(102400, 'a');
    EXPECT_TRUE(PublishedProxyData::CheckAndCorrectProxyData(proxyData, config100k));
    proxyData.value_ = std::string(102401, 'a');
    EXPECT_FALSE(PublishedProxyData::CheckAndCorrectProxyData(proxyData, config100k));
    ZLOGI("CheckAndCorrectProxyData006 end");
}

/**
 * @tc.name: CheckAndCorrectProxyData007
 * @tc.desc: Verify CheckAndCorrectProxyData sanitizes oversized trustProviders entries
 * @tc.type: FUNC
 * @tc.step:
 *   1. Set trustProviders with one entry exceeding APPIDENTIFIER_MAX_SIZE
 *   2. Call CheckAndCorrectProxyData -> true
 *   3. Verify oversized entry removed, valid entries preserved (size=2)
 */
HWTEST_F(DataShareProxyDataMgrTest, CheckAndCorrectProxyData007, TestSize.Level1)
{
    ZLOGI("CheckAndCorrectProxyData007 start");
    DataShareProxyData proxyData;
    proxyData.uri_ = "datashareproxy://com.test/test";
    proxyData.value_ = "value";
    proxyData.allowList_ = {"app1"};
    proxyData.trustProviders_ = {
        "app1",
        std::string(APPIDENTIFIER_MAX_SIZE + 1, 'x'),
        "app2"
    };

    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    config.maxValueLength_ = DataProxyMaxValueLength::MAX_LENGTH_4K;

    bool result = PublishedProxyData::CheckAndCorrectProxyData(proxyData, config);
    EXPECT_TRUE(result);
    EXPECT_EQ(proxyData.trustProviders_.size(), 2u);
    EXPECT_EQ(proxyData.trustProviders_[0], "app1");
    EXPECT_EQ(proxyData.trustProviders_[1], "app2");
    ZLOGI("CheckAndCorrectProxyData007 end");
}

} // namespace OHOS::Test
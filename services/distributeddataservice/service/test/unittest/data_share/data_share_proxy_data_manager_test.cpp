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

} // namespace OHOS::Test
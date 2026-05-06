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
#define LOG_TAG "DataShareServiceImplMockTest"

#include "data_share_service_impl.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstoken_kit.h"
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
#include "mock/native_and_hap_token_mock.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
std::string SLIENT_ACCESS_URI = "datashareproxy://com.acts.ohos.data.datasharetest/test";
std::string SA_SLIENT_ACCESS_URI = "datashareproxy://com.acts.ohos.data.datasharetest/SAID=12321/storeName/tableName";
std::string TBL_NAME0 = "name0";
std::string TBL_NAME1 = "name1";
std::string STORAGE_MANAGER_PROCE_NAME = "storage_manager";
namespace OHOS::Test {
using OHOS::DataShare::LogLabel;
class DataShareServiceImplMockTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static constexpr int32_t TEST_SA_ID = 12321;
    static void SetUpTestCase()
    {
        MockToken::SetTestEnvironment();
    }
    static void TearDownTestCase()
    {
        MockToken::ResetTestEnvironment();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: GetCallerInfo001
 * @tc.desc: Verify GetCallerInfo behavior with TOKEN_HAP and mock setup
 * @tc.type: FUNC
 * @tc.precon: DataShareServiceImpl and MockNativeToken are initialized
 * @tc.step:
 *   1. Set bundleName to empty
 *   2. Call GetCallerInfo with TOKEN_HAP, expect bundleName to be set
 * @tc.expect:
 *   1. Return true
 *   2. bundleName is STORAGE_MANAGER_PROCE_NAME
 */
HWTEST_F(DataShareServiceImplMockTest, GetCallerInfo001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplMockTest GetCallerInfo001 start");
    MockNativeToken mock(STORAGE_MANAGER_PROCE_NAME);
    DataShareServiceImpl dataShareServiceImpl;
    int32_t appIndex = 1;
    std::string bundleName = "";
    // TOKEN_HAP bundleName is ohos.datasharetest.demo
    auto result = dataShareServiceImpl.GetCallerInfo(bundleName, appIndex, TEST_SA_ID);
    EXPECT_TRUE(result.first);
    EXPECT_EQ(bundleName, STORAGE_MANAGER_PROCE_NAME);
    ZLOGI("DataShareServiceImplMockTest GetCallerInfo001 end");
}

/**
 * @tc.name: CheckAllowList001
 * @tc.desc: Verify CheckAllowList behavior with specific allow list configuration
 * @tc.type: FUNC
 * @tc.precon: DataShareServiceImpl and MockNativeToken are initialized
 * @tc.step:
 *   1. Set testTokenId for log
 *   2. Create allowLists with two entries, one with onlyMain set to true
 *   3. Call CheckAllowList and verify result
 * @tc.expect:
 *   1. Return true
 */
HWTEST_F(DataShareServiceImplMockTest, CheckAllowList001, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplMockTest CheckAllowList001 start");
    MockNativeToken mock(STORAGE_MANAGER_PROCE_NAME);
    DataShareServiceImpl dataShareServiceImpl;
    // testTokenId just for log
    uint32_t testTokenId = 0;
    AllowList procAllowList;
    std::vector<AllowList> allowLists;
    procAllowList.appIdentifier = "processName";
    procAllowList.onlyMain = true;
    allowLists.push_back(procAllowList);
    procAllowList.appIdentifier = STORAGE_MANAGER_PROCE_NAME;
    allowLists.push_back(procAllowList);
    // TOKEN_NATIVE and systemAbilityId is valid
    auto result = dataShareServiceImpl.CheckAllowList(USER_TEST, testTokenId, allowLists, TEST_SA_ID);
    EXPECT_TRUE(result);
    ZLOGI("DataShareServiceImplMockTest CheckAllowList001 end");
}

/**
 * @tc.name: CheckAllowList002
 * @tc.desc: Verify CheckAllowList behavior with specific allow list configuration
 * @tc.type: FUNC
 * @tc.precon: DataShareServiceImpl and MockNativeToken are initialized
 * @tc.step:
 *   1. Set testTokenId for log
 *   2. Create allowLists with one entry, onlyMain set to true
 *   3. Call CheckAllowList and verify result
 * @tc.expect:
 *   1. Return true
 */
HWTEST_F(DataShareServiceImplMockTest, CheckAllowList002, TestSize.Level1)
{
    ZLOGI("DataShareServiceImplMockTest CheckAllowList002 start");
    MockNativeToken mock(STORAGE_MANAGER_PROCE_NAME);
    DataShareServiceImpl dataShareServiceImpl;
    AllowList procAllowList;
    procAllowList.appIdentifier = "processName";
    procAllowList.onlyMain = true;
    std::vector<AllowList> allowLists = { procAllowList };
    // testTokenId just for log
    uint32_t testTokenId = 0;
    // TOKEN_NATIVE but systemAbilityId is INVALID_SA_ID
    auto result = dataShareServiceImpl.CheckAllowList(USER_TEST, testTokenId, allowLists, 0);
    EXPECT_TRUE(result);
    ZLOGI("DataShareServiceImplMockTest CheckAllowList002 end");
}
} // namespace OHOS::Test
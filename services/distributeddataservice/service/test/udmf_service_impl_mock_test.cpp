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

#define LOG_TAG "UdmfServiceImplMockTest"
#include "udmf_service_impl.h"
#include "gtest/gtest.h"
#include "error_code.h"
#include "text.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "executor_pool.h"
#include "ipc_skeleton.h"
#include "mock/access_token_mock.h"
#include "mock/meta_data_manager_mock.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "kvstore_meta_manager.h"
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using namespace testing::ext;
using namespace testing;

namespace OHOS::Test {
namespace DistributedDataTest {
class UdmfServiceImplMockTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        accTokenMock = std::make_shared<AccessTokenKitMock>();
        BAccessTokenKit::accessTokenkit = accTokenMock;
        metaDataManagerMock = std::make_shared<MetaDataManagerMock>();
        BMetaDataManager::metaDataManager = metaDataManagerMock;
        metaDataMock = std::make_shared<MetaDataMock<StoreMetaData>>();
        BMetaData<StoreMetaData>::metaDataManager = metaDataMock;
    }
    static void TearDownTestCase(void)
    {
        accTokenMock = nullptr;
        BAccessTokenKit::accessTokenkit = nullptr;
        metaDataManagerMock = nullptr;
        BMetaDataManager::metaDataManager = nullptr;
        metaDataMock = nullptr;
        BMetaData<StoreMetaData>::metaDataManager = nullptr;
    }
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    void SetUp() {};
    void TearDown() {};
};

/**
* @tc.name: IsNeedMetaSyncTest001
* @tc.desc: IsNeedMetaSync test
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplMockTest, IsNeedMetaSyncTest001, TestSize.Level0)
{
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillOnce(Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillOnce(Return(false));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillOnce(Return(true))
        .WillOnce(Return(true));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, false);
}

/**
* @tc.name: IsNeedMetaSyncTest002
* @tc.desc: IsNeedMetaSync test matrix mask
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplMockTest, IsNeedMetaSyncTest002, TestSize.Level0)
{
    QueryOption query;
    query.key = "test_key";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillOnce(Return(false))
        .WillOnce(Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);
    // mock mask
}

/**
 * @tc.name: ResolveAutoLaunchTest001
 * @tc.desc: ResolveAutoLaunch test.
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplMockTest, ResolveAutoLaunchTest001, TestSize.Level0)
{
    DistributedDB::AutoLaunchParam param {
        .userId = "100",
        .appId = "distributeddata",
        .storeId = "drag",
    };
    std::string identifier = "identifier";
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    auto ret = udmfServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, UDMF::E_NOT_FOUND);
}
}; // DistributedDataTest
}; // namespace OHOS::Test
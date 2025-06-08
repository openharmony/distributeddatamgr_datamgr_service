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

#include "metadata/meta_data_manager.h"
#define LOG_TAG "UdmfServiceImplTest"
#include "udmf_service_impl.h"
#include "gtest/gtest.h"
#include "error_code.h"
#include "text.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
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
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using namespace testing::ext;

namespace OHOS::Test {
namespace DistributedDataTest {
class UdmfServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    void SetUp() {}
    void TearDown() {}
};
void UdmfServiceImplTest::SetUpTestCase() 
{
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    metaDataManagerMock = std::make_shared<MetaDataManagerMock>();
    BMetaDataManager::metaDataManager = metaDataManagerMock;
    metaDataMock = std::make_shared<MetaDataMock<StoreMetaData>>();
    BMetaData<StoreMetaData>::metaDataManager = metaDataMock;
}

void UdmfServiceImplTest::TearDownTestCase(void)
{
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
    metaDataManagerMock = nullptr;
    BMetaDataManager::metaDataManager = nullptr;
    metaDataMock = nullptr;
    BMetaData<StoreMetaData>::metaDataManager = nullptr;
}

/**
* @tc.name: TransferToEntriesIfNeedTest001
* @tc.desc: TransferToEntriesIfNeed test
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, TransferToEntriesIfNeedTest001, TestSize.Level0)
{
    UnifiedData data;
    QueryOption query;
    auto record1 = std::make_shared<UnifiedRecord>();
    auto record2 = std::make_shared<UnifiedRecord>();
    data.AddRecord(record1);
    data.AddRecord(record2);
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->tag = "records_to_entries_data_format";
    data.SetProperties(properties);
    query.tokenId = 1;
    UdmfServiceImpl udmfServiceImpl;
    udmfServiceImpl.TransferToEntriesIfNeed(query, data);
    EXPECT_TRUE(data.IsNeedTransferToEntries());
    int recordSize = 2;
    EXPECT_EQ(data.GetRecords().size(), recordSize);
}

/**
* @tc.name: IsNeedMetaSyncTest001
* @tc.desc: IsNeedMetaSync test
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, IsNeedMetaSyncTest001, TestSize.Level0)
{
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(true));
    isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, false);
}

/**
* @tc.name: SyncTest001
* @tc.desc: IsNeedMetaSync test matrix mask
* @tc.type: FUNC
*/
HWTEST_F(UdmfServiceImplTest, IsNeedMetaSyncTest002, TestSize.Level0)
{
    QueryOption query;
    query.key = "test_key";
    query.tokenId = 1;
    query.intention  = UD_INTENTION_DRAG;
    UdmfServiceImpl udmfServiceImpl;
    StoreMetaData meta = StoreMetaData("100", "distributeddata", "drag");
    std::vector<std::string> devices = {"remote_device"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    auto isNeedSync = udmfServiceImpl.IsNeedMetaSync(meta, devices);
    EXPECT_EQ(isNeedSync, true);
    // mock mask
}

/**
 * @tc.name: ResolveAutoLaunchTest001
 * @tc.desc: ResolveAutoLaunch test.
 * @tc.type: FUNC
 */
HWTEST_F(UdmfServiceImplTest, ResolveAutoLaunchTest001, TestSize.Level0)
{
    auto store = StoreCache::GetInstance().GetStore("drag");
    auto ret = store->Init();
    EXPECT_EQ(ret, UDMF::E_OK);

    DistributedDB::AutoLaunchParam param {
        .userId = "100",
        .appId = "distributeddata",
        .storeId = "drag",
    };
    std::string identifier = "identifier";
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    ret = udmfServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, UDMF::E_NOT_FOUND);
}
} // DistributedDataTest
}; 
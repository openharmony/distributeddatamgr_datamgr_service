/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "session_manager/session_manager.h"

#include <cstdint>

#include "accesstoken_kit.h"
#include "account_delegate_mock.h"
#include "auth_delegate_mock.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "device_manager_adapter_mock.h"
#include "gtest/gtest.h"
#include <string>
#include "meta_data_manager_mock.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "nativetoken_kit.h"
#include "session_manager/route_head_handler_impl.h"
#include "session_manager/upgrade_manager.h"
#include "token_setproc.h"
#include "user_delegate.h"
#include "user_delegate_mock.h"
#include "utils/endian_converter.h"

namespace {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS;
using namespace OHOS::Security::AccessToken;
using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
using UserInfo = DistributedDB::UserInfo;
constexpr const char *PEER_DEVICE_ID = "PEER_DEVICE_ID";
constexpr const char *DDMS = "distributeddata";
constexpr const char *META_DB = "service_meta";
constexpr const char *DRAG = "drag";
constexpr const char *OTHER_APP_ID = "test_app_id";
constexpr const char *USER_ID = "100";
static constexpr int32_t OH_OS_TYPE = 10;

void GrantPermissionNative()
{
    const char **perms = new const char *[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class RouteHeadHandlerImplTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GrantPermissionNative();
        deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
        BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
        metaDataManagerMock = std::make_shared<MetaDataManagerMock>();
        BMetaDataManager::metaDataManager = metaDataManagerMock;
        metaDataMock = std::make_shared<MetaDataMock<StoreMetaData>>();
        BMetaData<StoreMetaData>::metaDataManager = metaDataMock;
        userDelegateMock = std::make_shared<UserDelegateMock>();
        BUserDelegate::userDelegate = userDelegateMock;
    }
    static void TearDownTestCase()
    {
        deviceManagerAdapterMock = nullptr;
        BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
        metaDataManagerMock = nullptr;
        BMetaDataManager::metaDataManager = nullptr;
        metaDataMock = nullptr;
        BMetaData<StoreMetaData>::metaDataManager = nullptr;
        userDelegateMock = nullptr;
        BUserDelegate::userDelegate = nullptr;
    }
    void SetUp(){};
    void TearDown(){};
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    static inline std::shared_ptr<UserDelegateMock> userDelegateMock = nullptr;
private:
    void GetEmptyHeadDataLen(const DistributedDB::ExtendInfo& info);
    void ParseEmptyHeadDataLen(const DistributedDB::ExtendInfo& info);
};

void RouteHeadHandlerImplTest::GetEmptyHeadDataLen(const DistributedDB::ExtendInfo& info)
{
    DeviceInfo deviceInfo;
    deviceInfo.osType = OH_OS_TYPE;
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceInfo(_)).WillRepeatedly(Return(deviceInfo));

    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::UDMF_AND_OBJECT_VERSION;
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(capMetaData), Return(true)));
    uint32_t headSize = 0;
    auto status = sendHandler->GetHeadDataSize(headSize);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_EQ(headSize, 0);
}

void RouteHeadHandlerImplTest::ParseEmptyHeadDataLen(const DistributedDB::ExtendInfo& info)
{
    DeviceInfo deviceInfo;
    deviceInfo.osType = OH_OS_TYPE;
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceInfo(_)).WillRepeatedly(Return(deviceInfo));

    auto recvHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(recvHandler, nullptr);

    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::UDMF_AND_OBJECT_VERSION;
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(capMetaData), Return(true)));
    
    uint32_t routeHeadSize = 0;
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(routeHeadSize);
    uint32_t parseSize = 0;
    auto status = recvHandler->ParseHeadDataLen(data.get(), routeHeadSize, parseSize, PEER_DEVICE_ID);
    EXPECT_EQ(status, false);
    EXPECT_EQ(parseSize, routeHeadSize);
}

/**
  * @tc.name: GetEmptyHeadDataLen_Test1
  * @tc.desc: test get udmf store
  * @tc.type: FUNC
  */
HWTEST_F(RouteHeadHandlerImplTest, GetEmptyHeadDataLen_Test1, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = DDMS, .storeId = DRAG, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}

/**
  * @tc.name: GetEmptyHeadDataLen_Test2
  * @tc.desc: test meta db
  * @tc.type: FUNC
  */
HWTEST_F(RouteHeadHandlerImplTest, GetEmptyHeadDataLen_Test2, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = DDMS, .storeId = META_DB, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}

/**
  * @tc.name: ParseEmptyHeadDataLen_Test1
  * @tc.desc: test get udmf store
  * @tc.type: FUNC
  */
HWTEST_F(RouteHeadHandlerImplTest, ParseEmptyHeadDataLen_Test1, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = DDMS, .storeId = DRAG, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}

/**
  * @tc.name: ParseEmptyHeadDataLen_Test2
  * @tc.desc: test get meta db
  * @tc.type: FUNC
  */
HWTEST_F(RouteHeadHandlerImplTest, ParseEmptyHeadDataLen_Test2, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = DDMS, .storeId = META_DB, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}

/**
  * @tc.name: ParseEmptyHeadDataLen_Test3
  * @tc.desc: test OTHER_APP_ID
  * @tc.type: FUNC
  */
HWTEST_F(RouteHeadHandlerImplTest, ParseEmptyHeadDataLen_Test3, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = OTHER_APP_ID, .storeId = DRAG, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}

/**
  * @tc.name: ParseEmptyHeadDataLen_Test4
  * @tc.desc: test get OTHER_APP_ID and meta db
  * @tc.type: FUNC

  */
HWTEST_F(RouteHeadHandlerImplTest, ParseEmptyHeadDataLen_Test4, TestSize.Level0)
{
    const DistributedDB::ExtendInfo info = {
        .appId = OTHER_APP_ID, .storeId = META_DB, .userId = USER_ID, .dstTarget = PEER_DEVICE_ID
    };
    GetEmptyHeadDataLen(info);
}
} // namespace
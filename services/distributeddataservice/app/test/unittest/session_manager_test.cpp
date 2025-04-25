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

#include "accesstoken_kit.h"
#include "account_delegate_mock.h"
#include "auth_delegate_mock.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "device_manager_adapter_mock.h"
#include "gtest/gtest.h"
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
constexpr int PEER_USER_ID1 = 101;
constexpr int PEER_USER_ID2 = 100;
constexpr int METADATA_UID = 2000000;
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

class SessionManagerTest : public testing::Test {
public:
    void CreateUserStatus(std::vector<UserStatus> &users)
    {
        UserStatus stat;
        stat.id = 0;
        UserStatus stat1;
        stat.id = 1;
        UserStatus stat2;
        stat.id = 2;
        UserStatus stat3;
        stat.id = 3;
        users.push_back(stat);
        users.push_back(stat1);
        users.push_back(stat2);
        users.push_back(stat3);
    }
    void CreateStoreMetaData(std::vector<StoreMetaData> &datas, SessionPoint local)
    {
        StoreMetaData data;
        data.appId = local.appId;
        data.storeId = local.storeId;
        data.bundleName = "com.test.session";
        StoreMetaData data1;
        data1.appId = local.appId;
        data1.storeId = "local.storeId";
        data1.bundleName = "com.test.session1";
        StoreMetaData data2;
        data2.appId = "local.appId";
        data2.storeId = local.storeId;
        data2.bundleName = "com.test.session2";
        StoreMetaData data3;
        data3.appId = "local.appId";
        data3.storeId = "local.storeId";
        data3.bundleName = "com.test.session3";
        datas.push_back(data);
        datas.push_back(data1);
        datas.push_back(data2);
        datas.push_back(data3);
    }
    static void SetUpTestCase()
    {
        auto executors = std::make_shared<ExecutorPool>(12, 5);
        Bootstrap::GetInstance().LoadComponents();
        Bootstrap::GetInstance().LoadDirectory();
        Bootstrap::GetInstance().LoadCheckers();
        DeviceManagerAdapter::GetInstance().Init(executors);

        // init peer device
        UserMetaData userMetaData;
        userMetaData.deviceId = PEER_DEVICE_ID;

        UserStatus status;
        status.isActive = true;
        status.id = PEER_USER_ID1;
        userMetaData.users = { status };
        status.id = PEER_USER_ID2;
        userMetaData.users.emplace_back(status);

        auto peerUserMetaKey = UserMetaRow::GetKeyFor(userMetaData.deviceId);
        MetaDataManager::GetInstance().SaveMeta({ peerUserMetaKey.begin(), peerUserMetaKey.end() }, userMetaData);

        CapMetaData capMetaData;
        capMetaData.version = CapMetaData::CURRENT_VERSION;

        auto peerCapMetaKey = CapMetaRow::GetKeyFor(userMetaData.deviceId);
        MetaDataManager::GetInstance().SaveMeta({ peerCapMetaKey.begin(), peerCapMetaKey.end() }, capMetaData);

        StoreMetaData metaData;
        metaData.bundleName = "ohos.test.demo";
        metaData.appId = "ohos.test.demo";
        metaData.storeId = "test_store";
        metaData.user = "100";
        metaData.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
        metaData.tokenId = AccessTokenKit::GetHapTokenID(PEER_USER_ID2, "ohos.test.demo", 0);
        metaData.uid = METADATA_UID;
        metaData.storeType = 1;
        MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);
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
        auto peerUserMetaKey = UserMetaRow::GetKeyFor(PEER_DEVICE_ID);
        MetaDataManager::GetInstance().DelMeta(std::string(peerUserMetaKey.begin(), peerUserMetaKey.end()));
        auto peerCapMetaKey = CapMetaRow::GetKeyFor(PEER_DEVICE_ID);
        MetaDataManager::GetInstance().DelMeta(std::string(peerCapMetaKey.begin(), peerCapMetaKey.end()));
        StoreMetaData metaData;
        metaData.bundleName = "ohos.test.demo";
        metaData.appId = "ohos.test.demo";
        metaData.storeId = "test_store";
        metaData.user = "100";
        metaData.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
        metaData.tokenId = AccessTokenKit::GetHapTokenID(PEER_USER_ID2, "ohos.test.demo", 0);
        metaData.uid = METADATA_UID;
        metaData.storeType = 1;
        MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
        deviceManagerAdapterMock = nullptr;
        BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
        metaDataManagerMock = nullptr;
        BMetaDataManager::metaDataManager = nullptr;
        metaDataMock = nullptr;
        BMetaData<StoreMetaData>::metaDataManager = nullptr;
        userDelegateMock = nullptr;
        BUserDelegate::userDelegate = nullptr;
    }
    void SetUp()
    {
        ConstructValidData();
    }
    void TearDown()
    {
    }
    void ConstructValidData()
    {
        constexpr size_t BUFFER_SIZE = sizeof(dataBuffer);
        memset_s(dataBuffer, BUFFER_SIZE, 0, BUFFER_SIZE);

        RouteHead head{};
        head.magic = RouteHead::MAGIC_NUMBER;
        head.version = RouteHead::VERSION;
        head.dataLen = sizeof(SessionDevicePair) + sizeof(SessionUserPair) + sizeof(uint32_t) * 1
                       + sizeof(SessionAppId) + APP_STR_LEN;
        head.checkSum = 0;

        uint8_t *ptr = dataBuffer;
        size_t remaining = BUFFER_SIZE;

        // 1. write RouteHead
        errno_t err = memcpy_s(ptr, remaining, &head, sizeof(RouteHead));
        ASSERT_EQ(err, 0) << "Failed to copy RouteHead";
        ptr += sizeof(RouteHead);
        remaining -= sizeof(RouteHead);

        // 2. write SessionDevicePair
        SessionDevicePair devPair{};
        constexpr size_t DEV_ID_SIZE = sizeof(devPair.sourceId);
        err = memset_s(devPair.sourceId, DEV_ID_SIZE, 'A', DEV_ID_SIZE - 1);
        ASSERT_EQ(err, 0) << "Failed to init sourceId";
        devPair.sourceId[DEV_ID_SIZE - 1] = '\0';

        err = memset_s(devPair.targetId, DEV_ID_SIZE, 'B', DEV_ID_SIZE - 1);
        ASSERT_EQ(err, 0) << "Failed to init targetId";
        devPair.targetId[DEV_ID_SIZE - 1] = '\0';

        err = memcpy_s(ptr, remaining, &devPair, sizeof(SessionDevicePair));
        ASSERT_EQ(err, 0) << "Failed to copy SessionDevicePair";
        ptr += sizeof(SessionDevicePair);
        remaining -= sizeof(SessionDevicePair);

        // 3. write SessionUserPair
        SessionUserPair userPair{};
        userPair.sourceUserId = HostToNet(100U);
        userPair.targetUserCount = HostToNet(1U);

        err = memcpy_s(ptr, remaining, &userPair, sizeof(SessionUserPair));
        ASSERT_EQ(err, 0) << "Failed to copy SessionUserPair";
        ptr += sizeof(SessionUserPair);
        remaining -= sizeof(SessionUserPair);
        uint32_t targetUser = HostToNet(200U);
        err = memcpy_s(ptr, remaining, &targetUser, sizeof(uint32_t));
        ASSERT_EQ(err, 0) << "Failed to copy targetUser";
        ptr += sizeof(uint32_t);
        remaining -= sizeof(uint32_t);

        // 4. write SessionAppId
        SessionAppId appId{};
        const char *appStr = "test";

        appId.len = HostToNet(static_cast<uint32_t>(APP_STR_LEN));

        err = memcpy_s(ptr, remaining, &appId, sizeof(SessionAppId));
        ASSERT_EQ(err, 0) << "Failed to copy SessionAppId";
        ptr += sizeof(SessionAppId);
        remaining -= sizeof(SessionAppId);

        err = memcpy_s(ptr, remaining, appStr, APP_STR_LEN);
        ASSERT_EQ(err, 0) << "Failed to copy appId data";
        ptr += APP_STR_LEN;
        remaining -= APP_STR_LEN;
    }
    const size_t validTotalLen = sizeof(RouteHead) + sizeof(SessionDevicePair) + sizeof(SessionUserPair)
                                 + sizeof(uint32_t) * 1 + sizeof(SessionAppId) + APP_STR_LEN;
    uint8_t dataBuffer[1024];
    static constexpr size_t APP_STR_LEN = 5;
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    static inline std::shared_ptr<UserDelegateMock> userDelegateMock = nullptr;
};

/**
  * @tc.name: PackAndUnPack01
  * @tc.desc: test get db dir
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: illybyy
  */
HWTEST_F(SessionManagerTest, PackAndUnPack01, TestSize.Level2)
{
    const DistributedDB::ExtendInfo info = {
        .appId = "ohos.test.demo", .storeId = "test_store", .userId = "100", .dstTarget = PEER_DEVICE_ID
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);
    uint32_t routeHeadSize = 0;
    sendHandler->GetHeadDataSize(routeHeadSize);
    ASSERT_EQ(routeHeadSize, 0);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(routeHeadSize);
    sendHandler->FillHeadData(data.get(), routeHeadSize, routeHeadSize);

    std::vector<DistributedDB::UserInfo> users;
    auto recvHandler = RouteHeadHandlerImpl::Create({});
    ASSERT_NE(recvHandler, nullptr);
    uint32_t parseSize = 1;
    auto res = recvHandler->ParseHeadDataLen(nullptr, routeHeadSize, parseSize);
    EXPECT_EQ(res, false);
    recvHandler->ParseHeadDataLen(data.get(), routeHeadSize, parseSize);
    EXPECT_EQ(routeHeadSize, parseSize);
    recvHandler->ParseHeadDataUser(data.get(), routeHeadSize, "", users);
    ASSERT_EQ(users.size(), 0);
}
/**
  * @tc.name: GetHeadDataSize_Test1
  * @tc.desc: test appId equal processLabel.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, GetHeadDataSize_Test1, TestSize.Level1)
{
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t headSize = 0;
    routeHeadHandlerImpl.appId_ = Bootstrap::GetInstance().GetProcessLabel();
    auto status = routeHeadHandlerImpl.GetHeadDataSize(headSize);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_EQ(headSize, 0);
}
/**
  * @tc.name: GetHeadDataSize_Test2
  * @tc.desc: test appId not equal processLabel.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, GetHeadDataSize_Test2, TestSize.Level1)
{
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t headSize = 0;
    routeHeadHandlerImpl.appId_ = "otherAppId";
    auto status = routeHeadHandlerImpl.GetHeadDataSize(headSize);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_EQ(headSize, 0);
}
/**
  * @tc.name: GetHeadDataSize_Test3
  * @tc.desc: test devInfo.osType equal OH_OS_TYPE, appId not equal processLabel.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, GetHeadDataSize_Test3, TestSize.Level1)
{
    DeviceInfo deviceInfo;
    deviceInfo.osType = OH_OS_TYPE;
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceInfo(_)).WillRepeatedly(Return(deviceInfo));
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t headSize = 0;
    routeHeadHandlerImpl.appId_ = "otherAppId";
    auto status = routeHeadHandlerImpl.GetHeadDataSize(headSize);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
    EXPECT_EQ(headSize, 0);
}
/**
  * @tc.name: GetHeadDataSize_Test4
  * @tc.desc: test GetHeadDataSize
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, GetHeadDataSize_Test4, TestSize.Level1)
{
    DeviceInfo deviceInfo;
    deviceInfo.osType = OH_OS_TYPE;
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceInfo(_)).WillRepeatedly(Return(deviceInfo));

    const DistributedDB::ExtendInfo info = {
        .appId = "otherAppId", .storeId = "test_store", .userId = "100", .dstTarget = PEER_DEVICE_ID
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::CURRENT_VERSION;
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(capMetaData), Return(true)));
    std::vector<UserStatus> userStatus;
    UserStatus userStat1;
    UserStatus userStat2;
    UserStatus userStat3;
    userStat1.id = 1;
    userStat2.id = 2;
    userStat3.id = 3;
    userStatus.push_back(userStat1);
    userStatus.push_back(userStat2);
    userStatus.push_back(userStat3);
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillRepeatedly(Return(userStatus));

    uint32_t headSize = 0;
    auto status = sendHandler->GetHeadDataSize(headSize);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_EQ(headSize, 0);

    uint32_t routeHeadSize = 10;
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(routeHeadSize);
    status = sendHandler->FillHeadData(data.get(), routeHeadSize, routeHeadSize);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}
/**
  * @tc.name: ParseHeadDataUserTest001
  * @tc.desc: test parse null data.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, ParseHeadDataUserTest001, TestSize.Level1)
{
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    const DistributedDB::ExtendInfo info = {
        .appId = "otherAppId", .storeId = "test_store", .userId = "100", .dstTarget = PEER_DEVICE_ID
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    uint32_t totalLen = 10;
    std::string label = "testLabel";
    std::vector<UserInfo> userInfos;

    bool result = sendHandler->ParseHeadDataUser(nullptr, totalLen, label, userInfos);

    EXPECT_FALSE(result);
    EXPECT_EQ(userInfos.size(), 0);
}
/**
  * @tc.name: ParseHeadDataUserTest002
  * @tc.desc: test totalLen < sizeof(RouteHead).
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, ParseHeadDataUserTest002, TestSize.Level1)
{
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    const DistributedDB::ExtendInfo info = {
        .appId = "otherAppId", .storeId = "test_store", .userId = "100", .dstTarget = PEER_DEVICE_ID
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    uint8_t data[10] = { 0 };
    std::string label = "testLabel";
    std::vector<UserInfo> userInfos;

    bool result = sendHandler->ParseHeadDataUser(data, sizeof(RouteHead) - 1, label, userInfos);

    EXPECT_FALSE(result);
    EXPECT_EQ(userInfos.size(), 0);
}

/**
  * @tc.name: ParseHeadDataUserTest003
  * @tc.desc: test totalLen < sizeof(RouteHead).
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, ParseHeadDataUserTest003, TestSize.Level1)
{
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    const DistributedDB::ExtendInfo info = {
        .appId = "otherAppId", .storeId = "test_store", .userId = "100", .dstTarget = PEER_DEVICE_ID
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    uint8_t data[10] = { 0 };
    std::string label = "testLabel";
    std::vector<UserInfo> userInfos;

    RouteHead head = { 0 };
    head.version = RouteHead::VERSION;
    head.dataLen = static_cast<uint32_t>(sizeof(data) - sizeof(RouteHead));

    bool result = sendHandler->ParseHeadDataUser(data, sizeof(RouteHead), label, userInfos);

    EXPECT_FALSE(result);
    EXPECT_EQ(userInfos.size(), 0);
}

/**
  * @tc.name: UnPackData_InvalidMagic
  * @tc.desc: test invalid magic.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, UnPackData_InvalidMagic, TestSize.Level1)
{
    RouteHead *head = reinterpret_cast<RouteHead *>(dataBuffer);
    head->magic = 0xFFFF;
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t unpackedSize;
    EXPECT_FALSE(routeHeadHandlerImpl.UnPackData(dataBuffer, validTotalLen, unpackedSize));
}

/**
  * @tc.name: UnPackData_VersionMismatch
  * @tc.desc: test version mismatch.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, UnPackData_VersionMismatch, TestSize.Level1)
{
    RouteHead *head = reinterpret_cast<RouteHead *>(dataBuffer);
    head->version = 0x00;
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t unpackedSize;
    EXPECT_FALSE(routeHeadHandlerImpl.UnPackData(dataBuffer, validTotalLen, unpackedSize));
}

/**
  * @tc.name: UnPackData_ValidData
  * @tc.desc: test valid data.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, UnPackData_ValidData, TestSize.Level1)
{
    ExtendInfo info;
    RouteHeadHandlerImpl routeHeadHandlerImpl(info);
    uint32_t unpackedSize;
    EXPECT_TRUE(routeHeadHandlerImpl.UnPackData(dataBuffer, validTotalLen, unpackedSize));
    EXPECT_EQ(unpackedSize, validTotalLen);
}

/**
  * @tc.name: ShouldAddSystemUserWhenLocalUserIdIsSystem
  * @tc.desc: test GetSession.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, ShouldAddSystemUserWhenLocalUserIdIsSystem, TestSize.Level1)
{
    SessionPoint local;
    local.userId = UserDelegate::SYSTEM_USER;
    local.appId = "test_app";
    local.deviceId = "local_device";
    local.storeId = "test_store";

    std::vector<UserStatus> users;
    CreateUserStatus(users);
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(users));
    EXPECT_CALL(AuthHandlerMock::GetInstance(), CheckAccess(_, _, _, _))
        .WillOnce(Return(std::pair(true, true)))
        .WillOnce(Return(std::pair(true, false)))
        .WillOnce(Return(std::pair(false, true)))
        .WillOnce(Return(std::pair(false, false)));
    std::vector<StoreMetaData> datas;
    CreateStoreMetaData(datas, local);
    EXPECT_CALL(*metaDataMock, LoadMeta(_, _, _)).WillRepeatedly(DoAll(SetArgReferee<1>(datas), Return(true)));

    Session session = SessionManager::GetInstance().GetSession(local, "target_device");
    ASSERT_EQ(2, session.targetUserIds.size());
    EXPECT_EQ(UserDelegate::SYSTEM_USER, session.targetUserIds[0]);
}

/**
  * @tc.name: ShouldReturnEarlyWhenGetSendAuthParamsFails
  * @tc.desc: test GetSession.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, ShouldReturnEarlyWhenGetSendAuthParamsFails, TestSize.Level1)
{
    SessionPoint local;
    local.userId = 100;
    local.appId = "test_app";
    local.deviceId = "local_device";
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(std::vector<UserStatus>{}));
    std::vector<StoreMetaData> datas;
    EXPECT_CALL(*metaDataMock, LoadMeta(_, _, _)).WillRepeatedly(Return(false));

    Session session = SessionManager::GetInstance().GetSession(local, "target_device");

    EXPECT_TRUE(session.targetUserIds.empty());
}

/**
  * @tc.name: CheckSession
  * @tc.desc: test CheckSession.
  * @tc.type: FUNC
  * @tc.author: guochao
  */
HWTEST_F(SessionManagerTest, CheckSession, TestSize.Level1)
{
    SessionPoint localSys;
    localSys.userId = UserDelegate::SYSTEM_USER;
    localSys.appId = "test_app";
    localSys.deviceId = "local_device";
    localSys.storeId = "test_store";
    SessionPoint localNormal;
    localNormal.userId = 100;
    localNormal.appId = "test_app";
    localNormal.deviceId = "local_device";
    localNormal.storeId = "test_store";
    std::vector<StoreMetaData> datas;
    CreateStoreMetaData(datas, localSys);
    EXPECT_CALL(*metaDataMock, LoadMeta(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(datas), Return(false)))
        .WillRepeatedly(DoAll(SetArgReferee<1>(datas), Return(true)));
    EXPECT_CALL(AuthHandlerMock::GetInstance(), CheckAccess(_, _, _, _))
        .WillOnce(Return(std::pair(false, true)))
        .WillOnce(Return(std::pair(true, false)));
    bool result = SessionManager::GetInstance().CheckSession(localSys, localNormal);
    EXPECT_FALSE(result);
    result = SessionManager::GetInstance().CheckSession(localSys, localNormal);
    EXPECT_FALSE(result);
    result = SessionManager::GetInstance().CheckSession(localNormal, localSys);
    EXPECT_TRUE(result);
}
} // namespace
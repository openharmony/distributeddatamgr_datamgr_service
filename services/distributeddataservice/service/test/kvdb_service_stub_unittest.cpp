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
#define LOG_TAG "KVDBServiceStubTest "

#include <gtest/gtest.h>

#include "bootstrap.h"
#include "device_matrix.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "kv_types_util.h"
#include "kvdb_service_impl.h"
#include "kvdb_service_stub.h"
#include "log_print.h"
#include "types.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using KVDBServiceStub = OHOS::DistributedKv::KVDBServiceStub;
using StoreId = OHOS::DistributedKv::StoreId;
using AppId = OHOS::DistributedKv::AppId;
using Options = OHOS::DistributedKv::Options;
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedKv.IKvStoreDataService";
static const StoreId STOREID = { "kvdb_test_storeid" };
static const AppId APPID = { "kvdb_test_appid" };
static const std::string HAPNAME = "testHap";
static const std::string INVALID_HAPNAME = "./testHap";
static const StoreId INVALID_STOREID = { "./kvdb_test_storeid" };
static const AppId INVALID_APPID = { "\\kvdb_test_appid" };

namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};
std::shared_ptr<DistributedKv::KVDBServiceImpl> kvdbServiceImpl = std::make_shared<DistributedKv::KVDBServiceImpl>();
std::shared_ptr<KVDBServiceStub> kvdbServiceStub = kvdbServiceImpl;

/**
 * @tc.name: OnRemoteRequest_Test_001
 * @tc.desc: Test OnRemoteRequest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnRemoteRequest, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    uint32_t code = 0;
    auto result = kvdbServiceStub->OnRemoteRequest(code, data, reply);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: OnRemoteRequest_Test_001
 * @tc.desc: Test OnRemoteRequest
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnRemoteRequest001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    uint32_t code = static_cast<uint32_t>(DistributedKv::KVDBServiceInterfaceCode::TRANS_HEAD) - 1;
    auto result = kvdbServiceStub->OnRemoteRequest(code, data, reply);
    EXPECT_EQ(result, -1);

    // Test code greater than TRANS_BUTT
    code = static_cast<uint32_t>(DistributedKv::KVDBServiceInterfaceCode::TRANS_BUTT);
    result = kvdbServiceStub->OnRemoteRequest(code, data, reply);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: GetStoreInfo
 * @tc.desc: Test GetStoreInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, GetStoreInfo, TestSize.Level1)
{
    MessageParcel data;
    auto [status, info] = kvdbServiceStub->GetStoreInfo(0, data);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: GetStoreInfo
 * @tc.desc: Test GetStoreInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, GetStoreInfo001, TestSize.Level1)
{
    MessageParcel data;
    AppId appId = {"test_app"};
    StoreId storeId = {"test_store"};
    auto [status, info] = kvdbServiceStub->GetStoreInfo(0, data);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
    EXPECT_EQ(info.bundleName, "");
    EXPECT_EQ(info.storeId, "");
}

/**
 * @tc.name: CheckPermission
 * @tc.desc: Test CheckPermission
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, CheckPermission, TestSize.Level1)
{
    KVDBServiceStub::StoreInfo info;
    info.bundleName = "test_bundleName";
    info.storeId = "test_storeId";
    uint32_t code = static_cast<uint32_t>(DistributedKv::KVDBServiceInterfaceCode::TRANS_PUT_SWITCH);
    auto status = kvdbServiceStub->CheckPermission(code, info);
    EXPECT_FALSE(status);

    code = static_cast<uint32_t>(DistributedKv::KVDBServiceInterfaceCode::TRANS_UNSUBSCRIBE_SWITCH_DATA);
    status = kvdbServiceStub->CheckPermission(code, info);
    EXPECT_FALSE(status);
}

/**
 * @tc.name: CheckPermission
 * @tc.desc: Test CheckPermission
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, CheckPermission001, TestSize.Level1)
{
    KVDBServiceStub::StoreInfo info;
    info.bundleName = "validApp";
    info.storeId = "validStore";
    EXPECT_FALSE(kvdbServiceStub->CheckPermission(0, info));
}


/**
 * @tc.name: OnBeforeCreate001
 * @tc.desc: Test OnBeforeCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnBeforeCreate001, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnBeforeCreate(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnBeforeCreate002
 * @tc.desc: Test OnBeforeCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnBeforeCreate002, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnBeforeCreate(INVALID_APPID, INVALID_STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnBeforeCreate003
 * @tc.desc: Test OnBeforeCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnBeforeCreate003, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = INVALID_HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnBeforeCreate(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnAfterCreate001
 * @tc.desc: Test OnAfterCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnAfterCreate001, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnAfterCreate(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnAfterCreate002
 * @tc.desc: Test OnAfterCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnAfterCreate002, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnAfterCreate(INVALID_APPID, INVALID_STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnAfterCreate003
 * @tc.desc: Test OnAfterCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnAfterCreate003, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    Options options;
    options.hapName = INVALID_HAPNAME;
    ITypesUtil::Marshal(data, options);
    auto status = kvdbServiceStub->OnAfterCreate(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnSync
 * @tc.desc: Test OnSync
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnSync, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    AppId appId = {"testAppId01"};
    StoreId storeId = {"testStoreId01"};
    auto status = kvdbServiceStub->OnSync(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnRegServiceNotifier
 * @tc.desc: Test OnRegServiceNotifier
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnRegServiceNotifier, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnRegServiceNotifier(appId, storeId, data, reply);
    EXPECT_EQ(status, ERR_NONE);
}

/**
 * @tc.name: OnSetSyncParam
 * @tc.desc: Test OnSetSyncParam
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnSetSyncParam, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnSetSyncParam(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnAddSubInfo
 * @tc.desc: Test OnAddSubInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnAddSubInfo, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnAddSubInfo(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);

    status = kvdbServiceStub->OnRmvSubInfo(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnRmvSubInfo
 * @tc.desc: Test OnRmvSubInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnRmvSubInfo, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnRmvSubInfo(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnPutSwitch
 * @tc.desc: Test OnPutSwitch
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnPutSwitch, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnPutSwitch(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);

    status = kvdbServiceStub->OnGetSwitch(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPassword
 * @tc.desc: Test OnGetBackupPassword
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPassword, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnGetBackupPassword(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnSubscribeSwitchData
 * @tc.desc: Test OnSubscribeSwitchData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnSubscribeSwitchData, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"appId"};
    StoreId storeId = {"storeId"};
    auto status = kvdbServiceStub->OnSubscribeSwitchData(appId, storeId, data, reply);
    EXPECT_EQ(status, ERR_NONE);

    status = kvdbServiceStub->OnUnsubscribeSwitchData(appId, storeId, data, reply);
    EXPECT_EQ(status, ERR_NONE);
}

/**
 * @tc.name: OnSetConfig
 * @tc.desc: Test OnSetConfig
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnSetConfig, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"appId"};
    StoreId storeId = {"storeId"};
    auto status = kvdbServiceStub->OnSetConfig(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnRemoveDeviceData
 * @tc.desc: Test OnRemoveDeviceData
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnRemoveDeviceData, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"appId"};
    StoreId storeId = {"storeId"};
    auto status = kvdbServiceStub->OnRemoveDeviceData(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDelete
 * @tc.desc: Test OnDelete
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDelete, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"appId"};
    StoreId storeId = {"storeId"};
    auto status = kvdbServiceStub->OnDelete(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptions
 * @tc.desc: Test OnDeleteByOptions
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptions, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"appId"};
    StoreId storeId = {"storeId"};
    auto status = kvdbServiceStub->OnDeleteByOptions(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithValidData
 * @tc.desc: Test OnDeleteByOptions with valid data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithValidData, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.isCustomDir = false;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithCustomDir
 * @tc.desc: Test OnDeleteByOptions with custom directory
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithCustomDir, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.isCustomDir = true;
    options.baseDir = "/data/custom/path";
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithInvalidAppId
 * @tc.desc: Test OnDeleteByOptions with invalid appId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithInvalidAppId, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(INVALID_APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithInvalidStoreId
 * @tc.desc: Test OnDeleteByOptions with invalid storeId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithInvalidStoreId, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, INVALID_STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithValidBackupInfo
 * @tc.desc: Test OnGetBackupPassword with valid BackupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithValidBackupInfo, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithCustomDir
 * @tc.desc: Test OnGetBackupPassword with custom directory
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithCustomDir, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup_custom";
    info.baseDir = "/data/custom/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = true;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 2;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithInvalidData
 * @tc.desc: Test OnGetBackupPassword with invalid data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithInvalidData, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithInvalidBackupInfo
 * @tc.desc: Test OnGetBackupPassword with invalid BackupInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithInvalidBackupInfo, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "";
    info.baseDir = "";
    info.appId = "";
    info.storeId = "";
    info.subUser = -1;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 0;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithMissingPasswordType
 * @tc.desc: Test OnGetBackupPassword without password type
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithMissingPasswordType, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteWithValidSubUser001
 * @tc.desc: Test OnDelete with valid subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteWithValidSubUser001, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    int32_t subUser = 0;
    ITypesUtil::Marshal(data, subUser);
    
    auto status = kvdbServiceStub->OnDelete(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteWithNegativeSubUser002
 * @tc.desc: Test OnDelete with negative subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteWithNegativeSubUser002, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    int32_t subUser = -1;
    ITypesUtil::Marshal(data, subUser);
    
    auto status = kvdbServiceStub->OnDelete(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteWithLargeSubUser003
 * @tc.desc: Test OnDelete with large subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteWithLargeSubUser003, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    int32_t subUser = 99999;
    ITypesUtil::Marshal(data, subUser);
    
    auto status = kvdbServiceStub->OnDelete(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteWithInvalidSubUser004
 * @tc.desc: Test OnDelete with invalid subUser data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteWithInvalidSubUser004, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    auto status = kvdbServiceStub->OnDelete(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithAllOptions005
 * @tc.desc: Test OnDeleteByOptions with all options set
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithAllOptions005, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.isCustomDir = true;
    options.baseDir = "/data/custom/path";
    options.autoSync = true;
    options.encrypt = false;
    options.backup = false;
    options.hapName = "test.hap";
    options.createIfMissing = true;
    options.securityLevel = OHOS::DistributedKv::S1;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithEmptyOptions006
 * @tc.desc: Test OnDeleteByOptions with empty options
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithEmptyOptions006, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithDeviceCollaboration007
 * @tc.desc: Test OnDeleteByOptions with device collaboration type
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithDeviceCollaboration007, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::DEVICE_COLLABORATION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 1;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithDifferentAreas008
 * @tc.desc: Test OnDeleteByOptions with different areas
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithDifferentAreas008, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL2;
    options.subUser = 0;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithDifferentPasswordTypes009
 * @tc.desc: Test OnGetBackupPassword with different password types
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithDifferentPasswordTypes009, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    for (int32_t type = 0; type < 4; type++) {
        MessageParcel testData;
        testData.WriteInterfaceToken(INTERFACE_TOKEN);
        ITypesUtil::Marshal(testData, info);
        ITypesUtil::Marshal(testData, type);
        
        auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, testData, reply);
        EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
    }
}

/**
 * @tc.name: OnGetBackupPasswordWithExtremePasswordType010
 * @tc.desc: Test OnGetBackupPassword with extreme password type values
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithExtremePasswordType010, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t extremeTypes[] = {INT32_MAX, INT32_MIN, 100, -100};
    for (auto passwordType : extremeTypes) {
        ITypesUtil::Marshal(data, passwordType);
        auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
        EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
    }
}

/**
 * @tc.name: OnGetBackupPasswordWithPartialBackupInfo011
 * @tc.desc: Test OnGetBackupPassword with partial BackupInfo fields
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithPartialBackupInfo011, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "";
    info.appId = "";
    info.storeId = "";
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithLargeSubUser012
 * @tc.desc: Test OnGetBackupPassword with large subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithLargeSubUser012, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 99999;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithNegativeSubUser013
 * @tc.desc: Test OnGetBackupPassword with negative subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithNegativeSubUser013, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = -100;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithLongBaseDir014
 * @tc.desc: Test OnDeleteByOptions with very long baseDir
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithLongBaseDir014, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.isCustomDir = true;
    options.baseDir = std::string(1000, 'a');
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithValidSubUser015
 * @tc.desc: Test OnGetBackupPassword with valid subUser values
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithValidSubUser015, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 1;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithMultipleFields016
 * @tc.desc: Test OnGetBackupPassword with all BackupInfo fields populated
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithMultipleFields016, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "multi_field_backup";
    info.baseDir = "/data/multi/backup/path";
    info.appId = "com.test.app";
    info.storeId = "multi_store_id";
    info.subUser = 10;
    info.isCustomDir = true;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 2;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithSecurityLevel017
 * @tc.desc: Test OnDeleteByOptions with different security levels
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithSecurityLevel017, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.securityLevel = OHOS::DistributedKv::S4;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithZeroSubUser018
 * @tc.desc: Test OnGetBackupPassword with zero subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithZeroSubUser018, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "test_backup";
    info.baseDir = "/data/backup";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = false;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 0;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteByOptionsWithAutoSyncOptions019
 * @tc.desc: Test OnDeleteByOptions with auto sync options
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteByOptionsWithAutoSyncOptions019, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    Options options;
    options.kvStoreType = OHOS::DistributedKv::SINGLE_VERSION;
    options.area = OHOS::DistributedKv::EL1;
    options.subUser = 0;
    options.autoSync = true;
    options.encrypt = true;
    options.backup = true;
    
    ITypesUtil::Marshal(data, options);
    
    auto status = kvdbServiceStub->OnDeleteByOptions(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnDeleteWithZeroSubUser020
 * @tc.desc: Test OnDelete with zero subUser
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnDeleteWithZeroSubUser020, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    int32_t subUser = 0;
    ITypesUtil::Marshal(data, subUser);
    
    auto status = kvdbServiceStub->OnDelete(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnGetBackupPasswordWithNullBaseDir021
 * @tc.desc: Test OnGetBackupPassword with null baseDir
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnGetBackupPasswordWithNullBaseDir021, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    
    BackupInfo info;
    info.name = "";
    info.baseDir = "/data/null/path";
    info.appId = APPID.appId;
    info.storeId = STOREID.storeId;
    info.subUser = 0;
    info.isCustomDir = true;
    
    ITypesUtil::Marshal(data, info);
    
    int32_t passwordType = 1;
    ITypesUtil::Marshal(data, passwordType);
    
    auto status = kvdbServiceStub->OnGetBackupPassword(APPID, STOREID, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

} // namespace DistributedDataTest
} // namespace OHOS::Test
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

#include "kvdb_service_stub.h"
#include "kvdb_service_impl.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "kv_types_util.h"
#include "log_print.h"
#include "types.h"
#include "device_matrix.h"
#include "bootstrap.h"
#include "itypes_util.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using KVDBServiceStub = OHOS::DistributedKv::KVDBServiceStub;
using StoreId = OHOS::DistributedKv::StoreId;
using AppId = OHOS::DistributedKv::AppId;
using Options = OHOS::DistributedKv::Options;
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedKv.IKvStoreDataService";
static OHOS::DistributedKv::StoreId storeId = { "kvdb_test_storeid" };
static OHOS::DistributedKv::AppId appId = { "ohos.test.kvdb" };
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
 * @tc.name: OnBeforeCreate
 * @tc.desc: Test OnBeforeCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnBeforeCreate, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStoreId"};
    auto status = kvdbServiceStub->OnBeforeCreate(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnAfterCreate
 * @tc.desc: Test OnAfterCreate
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnAfterCreate, TestSize.Level1)
{
    MessageParcel data;
    data.WriteInterfaceToken(INTERFACE_TOKEN);
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnAfterCreate(appId, storeId, data, reply);
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
 * @tc.name: OnSubscribe
 * @tc.desc: Test OnSubscribe
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KVDBServiceStubTest, OnSubscribe, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    AppId appId = {"testApp"};
    StoreId storeId = {"testStore"};
    auto status = kvdbServiceStub->OnSubscribe(appId, storeId, data, reply);
    EXPECT_EQ(status, IPC_STUB_INVALID_DATA_ERR);

    status = kvdbServiceStub->OnUnsubscribe(appId, storeId, data, reply);
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
} // namespace DistributedDataTest
} // namespace OHOS::Test
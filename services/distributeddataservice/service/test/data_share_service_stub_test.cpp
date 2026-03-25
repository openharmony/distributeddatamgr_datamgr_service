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
#define LOG_TAG "DataShareServiceStubTest"

#include "data_share_service_stub.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "data_share_service_impl.h"
#include "ipc_skeleton.h"
#include "token_setproc.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
const std::u16string INTERFACE_TOKEN = u"OHOS.DataShare.IDataShareService";
constexpr uint32_t CODE_MIN = 0;
constexpr size_t TEST_SIZE = 1;
constexpr uint8_t TEST_DATA = 1;
constexpr uint32_t CODE_MAX = IDataShareService::DATA_SHARE_SERVICE_CMD_MAX + 1;
constexpr uint64_t SYSTEM_TOKENID = static_cast<uint64_t>(1) << 32;
namespace OHOS::Test {
class DataShareServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};
std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;

/**
* @tc.name: OnRemoteRequest001
* @tc.desc: test OnRemoteRequest data error scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest001, TestSize.Level1)
{
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest001 start");
    auto originalToken = GetSelfTokenID();
    SetSelfTokenID(SYSTEM_TOKENID);
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnNotifyConnectDone(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_OK);
    SetSelfTokenID(originalToken);
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest001 end");
}

/**
* @tc.name: OnRemoteRequest002
* @tc.desc: test OnRemoteRequest abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest002, TestSize.Level1)
{
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest002 start");
    auto originalToken = GetSelfTokenID();
    SetSelfTokenID(SYSTEM_TOKENID);
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnNotifyConnectDone(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_OK);
    SetSelfTokenID(originalToken);
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest002 end");
}

/**
* @tc.name: OnRemoteRequest003
* @tc.desc: test OnRemoteRequest abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest003, TestSize.Level1)
{
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest003 start");
    uint32_t code = DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE;
    MessageParcel request;
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, E_NOT_SYSTEM_APP);

    code = DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE_SYSTEM;
    result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, E_NOT_SYSTEM_APP);

    ZLOGI("DataShareServiceStubTest::OnRemoteRequest003 end");
}

/**
* @tc.name: OnRemoteRequest004
* @tc.desc: test OnRemoteRequest abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest004, TestSize.Level1)
{
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest004 start");
    auto originalToken = GetSelfTokenID();
    SetSelfTokenID(SYSTEM_TOKENID);
    uint32_t code = DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_QUERY_SYSTEM;
    MessageParcel request;
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    SetSelfTokenID(originalToken);
    ZLOGI("DataShareServiceStubTest::OnRemoteRequest004 end");
}

HWTEST_F(DataShareServiceStubTest, IsTemplateRequest001, TestSize.Level1)
{
    ZLOGI("DataShareServiceStubTest::IsTemplateRequest001 start");
    static uint32_t list[] = {
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_ADD_TEMPLATE,
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_DEL_TEMPLATE,
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_SUBSCRIBE_RDB,
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_UNSUBSCRIBE_RDB,
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_ENABLE_SUBSCRIBE_RDB,
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_RDB,
    };
    for (auto code : list) {
        auto result = dataShareServiceStub->IsTemplateRequest(code);
        EXPECT_EQ(result, true);
    }

    auto result = dataShareServiceStub->IsTemplateRequest(
        DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_QUERY);
    EXPECT_EQ(result, false);

    // code out of range is false
    for (int32_t code = DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_DISABLE_SUBSCRIBE_RDB + 1;
        code <= DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_MAX; code++) {
        auto result = dataShareServiceStub->IsTemplateRequest(code);
        EXPECT_EQ(result, false);
    }
    ZLOGI("DataShareServiceStubTest::IsTemplateRequest001 end");
}

/**
* @tc.name: OnInsert001
* @tc.desc: test InsertEx UpdateEx Query DeleteEx function of abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnInsert001, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnInsertEx(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnUpdateEx(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnQuery(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnDeleteEx(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
* @tc.name: OnAddTemplate001
* @tc.desc: test OnAddTemplate function abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnAddTemplate001, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnAddTemplate(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnDelTemplate(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);
}

/**
* @tc.name: OnEnablePubSubs001
* @tc.desc: test OnEnablePubSubs function abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnEnablePubSubs001, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnEnablePubSubs(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnPublish(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnGetData(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnSubscribePublishedData(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnUnsubscribePublishedData(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnDisablePubSubs(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);
}

/**
* @tc.name: OnEnableRdbSubs001
* @tc.desc: test OnEnableRdbSubs function abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnEnableRdbSubs001, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnEnablePubSubs(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnPublish(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnEnableRdbSubs(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnSubscribeRdbData(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnUnsubscribeRdbData(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnDisableRdbSubs(request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);
}

/**
 * @tc.name: OnGetConnectionInterfaceInfo
 * @tc.desc: Verify OnRemoteRequest method of DataShareServiceImpl class with
 *           DATA_SHARE_SERVICE_CMD_GET_CONNECTION_INTERFACE_INFO command handles
 *           valid parameters correctly and returns DATA_SHARE_OK.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of DataShareServiceImpl and MessageParcel objects.
 *     2. The IDataShareServiceService class provides GetDescriptor and DATA_SHARE_OK constants.
 *     3. The DataShareServiceImpl class provides DATA_SHARE_SERVICE_CMD_GET_CONNECTION_INTERFACE_INFO constant.
 * @tc.step:
 *     1. Instantiate a DataShareServiceImpl object (service).
 *     2. Create empty MessageParcel objects (data, reply).
 *     3. Define interfaceCode (1001) and waitTime (10) as test parameters.
 *     4. Write interface descriptor, interfaceCode, and waitTime to data parcel.
 *     5. Set code to DATA_SHARE_SERVICE_CMD_GET_CONNECTION_INTERFACE_INFO.
 *     6. Call OnRemoteRequest with code, data, and reply; verify return equals DATA_SHARE_OK.
 * @tc.expect:
 *     1. OnRemoteRequest returns DATA_SHARE_OK for valid get connection interface info request.
 */
HWTEST_F(DataShareServiceStubTest, OnGetConnectionInterfaceInfo, TestSize.Level1)
{
    DataShareServiceImpl service;
    MessageParcel data;
    MessageParcel reply;
    int32_t interfaceCode = 1001; // 1001 is interfaceCode
    uint32_t waitTime = 10; // 10 is waitTime
    
    data.WriteInterfaceToken(IDataShareService::GetDescriptor());
    data.WriteInt32(interfaceCode);
    data.WriteUint32(waitTime);
    
    int32_t code = DataShare::DataShareServiceImpl::DATA_SHARE_SERVICE_CMD_GET_CONNECTION_INTERFACE_INFO;
    int32_t result = service.OnRemoteRequest(code, data, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_OK);
}
} // namespace OHOS::Test
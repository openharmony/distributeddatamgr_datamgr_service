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

#include <gtest/gtest.h>
#include <unistd.h>
#include "log_print.h"
#include "ipc_skeleton.h"
#include "data_share_service_impl.h"
#include "data_share_service_stub.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
const std::u16string INTERFACE_TOKEN = u"OHOS.DataShare.IDataShareService";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = IDataShareService::DATA_SHARE_SERVICE_CMD_MAX + 1;
namespace OHOS::Test {
class DataShareServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

/**
* @tc.name: OnRemoteRequest001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_NE(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnRemoteNotifyConnectDone(request, reply);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: OnRemoteRequest002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest002, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = CODE_MAX;
    uint8_t *data = &value;
    size_t size = 1;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnRemoteNotifyConnectDone(request, reply);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: OnRemoteRequest003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRequest003, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);

    result = dataShareServiceStub->OnRemoteNotifyConnectDone(request, reply);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: OnRemoteInsert001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteInsert001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteInsert(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteUpdate(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteQuery(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteDelete(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
* @tc.name: OnRemoteAddTemplate001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteAddTemplate001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteAddTemplate(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteDelTemplate(request, reply);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnRemoteEnablePubSubs001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteEnablePubSubs001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;

    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteEnablePubSubs(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemotePublish(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteGetData(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteSubscribePublishedData(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteUnsubscribePublishedData(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteEnableRdbSubs(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteDisableRdbSubs(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteDisablePubSubs(request, reply);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnRemoteEnableRdbSubs001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteEnableRdbSubs001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteEnablePubSubs(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemotePublish(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteEnableRdbSubs(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteSubscribeRdbData(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteUnsubscribeRdbData(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteDisableRdbSubs(request, reply);
    EXPECT_EQ(result, -1);
}

/**
* @tc.name: OnRemoteRegisterObserver001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceStubTest, OnRemoteRegisterObserver001, TestSize.Level1)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::shared_ptr<DataShareServiceStub> dataShareServiceStub = dataShareServiceImpl;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRegisterObserver(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteNotifyObserver(request, reply);
    EXPECT_EQ(result, -1);

    result = dataShareServiceStub->OnRemoteSetSilentSwitch(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteIsSilentProxyEnable(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = dataShareServiceStub->OnRemoteUnregisterObserver(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}
} // namespace OHOS::Test
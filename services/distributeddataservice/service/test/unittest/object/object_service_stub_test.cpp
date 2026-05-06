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

#define LOG_TAG "ObjectServiceStubTest"

#include "object_service_stub.h"

#include <gtest/gtest.h>

#include "ipc_skeleton.h"
#include "object_service_impl.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
namespace OHOS::Test {
const std::u16string INTERFACE_TOKEN = u"OHOS.DistributedObject.IObjectService";
constexpr uint32_t CODE_MAX = static_cast<uint32_t>(ObjectCode::OBJECTSTORE_SERVICE_CMD_MAX) + 1;
constexpr size_t TEST_SIZE = 1;
constexpr uint8_t TEST_DATA = 1;
class ObjectServiceStubTest : public testing::Test {
public:
    void SetUp(){};
    void TearDown(){};
};
std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
std::shared_ptr<ObjectServiceStub> objectServiceStub = objectServiceImpl;

/**
 * @tc.name: OnRemoteRequest001
 * @tc.desc: OnRemoteRequest data error test
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: shuxin
 */
HWTEST_F(ObjectServiceStubTest, ObjectServiceTest001, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    uint32_t code = 0;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;
    auto result = objectServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    code = CODE_MAX - 1;
    result = objectServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: OnRemoteRequest002
 * @tc.desc: OnRemoteRequest test
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: shuxin
 */
HWTEST_F(ObjectServiceStubTest, ObjectServiceTest002, TestSize.Level1)
{
    uint8_t value = TEST_DATA;
    uint8_t *data = &value;
    size_t size = TEST_SIZE;
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(data, size);
    request.RewindRead(0);
    MessageParcel reply;

    auto result = objectServiceStub->ObjectStoreRevokeSaveOnRemote(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->ObjectStoreRetrieveOnRemote(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnSubscribeRequest(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnUnsubscribeRequest(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnAssetChangedOnRemote(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->ObjectStoreBindAssetOnRemote(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnSubscribeProgress(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnUnsubscribeProgress(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnDeleteSnapshot(request, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);

    result = objectServiceStub->OnIsContinue(request, reply);
    EXPECT_EQ(result, 0);
}
}
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
#include "dump/dump_manager.h"
#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "token_setproc.h"
#include "message_parcel.h"
#include "idata_share_service.h"
#define private public
#include "data_share_service_stub.h"
#undef private

using namespace testing::ext;
using DumpManager = OHOS::DistributedData::DumpManager;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::DistributedKv;
const std::u16string INTERFACE_TOKEN = u"OHOS.DataShare.IDataShareService";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = IDataShareService::DATA_SHARE_SERVICE_CMD_MAX + 1;
namespace OHOS::Test {
class DataShareServiceStubTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static constexpr int64_t TEST_SUB_ID = 100;
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
    uint8_t value = 1;
    uint8_t *data = &value;
    uint32_t code = static_cast<uint32_t>(*data) % (CODE_MAX - CODE_MIN + 1) + CODE_MIN;
    MessageParcel request;
    MessageParcel reply;
    auto result = dataShareServiceStub->OnRemoteRequest(code, request, reply);
    EXPECT_EQ(result, IDataShareService::DATA_SHARE_ERROR);
}
} // namespace OHOS::Test
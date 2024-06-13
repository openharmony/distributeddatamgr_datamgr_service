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
#define LOG_TAG "RdbResultSetStubTest"

#include "gtest/gtest.h"
#include "log_print.h"
#include "message_parcel.h"
#include "rdb_result_set_impl.h"
#include "rdb_result_set_stub.h"
#include "store/cursor.h"
#include "securec.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
const std::u16string INTERFACE_TOKEN = u"OHOS::NativeRdb.IResultSet";
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbResultSetStubTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

/**
* @tc.name: OnRemoteRequest001
* @tc.desc: RdbResultSetStub OnRemoteRequest function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbResultSetStubTest, OnRemoteRequest001, TestSize.Level1)
{
    std::shared_ptr<DistributedData::Cursor> dbResultSet;
    auto result = std::make_shared<RdbResultSetImpl>(dbResultSet);
    MessageParcel request1;
    MessageParcel reply1;
    MessageOption option1;
    std::shared_ptr<RdbResultSetStub> rdbResultSetStub1 = std::make_shared<RdbResultSetStub>(result);
    auto ret = rdbResultSetStub1->OnRemoteRequest(RdbResultSetStub::Code::CMD_GET_ALL_COLUMN_NAMES,
        request1, reply1, option1);
    EXPECT_EQ(ret, -1);

    result = nullptr;
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request2;
    request2.WriteInterfaceToken(INTERFACE_TOKEN);
    request2.WriteBuffer(data, size);
    request2.RewindRead(0);
    MessageParcel reply2;
    MessageOption option2;
    std::shared_ptr<RdbResultSetStub> rdbResultSetStub2 = std::make_shared<RdbResultSetStub>(result);
    ret = rdbResultSetStub2->OnRemoteRequest(RdbResultSetStub::Code::CMD_GET_ALL_COLUMN_NAMES,
        request2, reply2, option2);
    EXPECT_EQ(ret, -1);

    MessageParcel request3;
    MessageParcel reply3;
    MessageOption option3;
    std::shared_ptr<RdbResultSetStub> rdbResultSetStub3 = std::make_shared<RdbResultSetStub>(result);
    ret = rdbResultSetStub3->OnRemoteRequest(RdbResultSetStub::Code::CMD_GET_ALL_COLUMN_NAMES,
        request3, reply3, option3);
    EXPECT_EQ(ret, -1);
}

/**
* @tc.name: OnRemoteRequest002
* @tc.desc: RdbResultSetStub OnRemoteRequest function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbResultSetStubTest, OnRemoteRequest002, TestSize.Level1)
{
    std::shared_ptr<DistributedData::Cursor> dbResultSet;
    auto result = std::make_shared<RdbResultSetImpl>(dbResultSet);
    std::shared_ptr<RdbResultSetStub> rdbResultSetStub = std::make_shared<RdbResultSetStub>(result);
    uint8_t value = 1;
    uint8_t *data = &value;
    size_t size = 1;
    MessageParcel request1;
    request1.WriteInterfaceToken(INTERFACE_TOKEN);
    request1.WriteBuffer(data, size);
    request1.RewindRead(0);
    MessageParcel reply1;
    MessageOption option1;
    uint32_t code = -1;
    auto ret = rdbResultSetStub->OnRemoteRequest(code, request1, reply1, option1);
    EXPECT_TRUE(ret > 0);

    code = 20; // invalid code > Code::CMD_MAX
    MessageParcel request2;
    request2.WriteInterfaceToken(INTERFACE_TOKEN);
    request2.WriteBuffer(data, size);
    request2.RewindRead(0);
    MessageParcel reply2;
    MessageOption option2;
    ret = rdbResultSetStub->OnRemoteRequest(code, request2, reply2, option2);
    EXPECT_TRUE(ret > 0);
}

/**
* @tc.name: RdbResultSetStub
* @tc.desc: RdbResultSetStub function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbResultSetStubTest, RdbResultSetStub, TestSize.Level1)
{
    std::shared_ptr<DistributedData::Cursor> dbResultSet;
    auto result = std::make_shared<RdbResultSetImpl>(dbResultSet);
    std::shared_ptr<RdbResultSetStub> rdbResultSetStub = std::make_shared<RdbResultSetStub>(result);
    for (uint32_t i = RdbResultSetStub::Code::CMD_GET_ALL_COLUMN_NAMES; i < RdbResultSetStub::Code::CMD_MAX; ++i)
    {
        RdbResultSetStub::Code code = static_cast<RdbResultSetStub::Code>(i);
        uint8_t value = 1;
        uint8_t *data = &value;
        size_t size = 1;
        MessageParcel request;
        request.WriteInterfaceToken(INTERFACE_TOKEN);
        request.WriteBuffer(data, size);
        request.RewindRead(0);
        MessageParcel reply;
        MessageOption option;
        auto ret = rdbResultSetStub->OnRemoteRequest(code, request, reply, option);
        EXPECT_EQ(ret, 0);
    }
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
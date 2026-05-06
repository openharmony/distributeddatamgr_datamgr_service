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

#define LOG_TAG "UtdServiceStubTest"
#include "utd_service_stub.h"
#include "utd_service_impl.h"
#include "gtest/gtest.h"
#include "utd_tlv_util.h"

using namespace OHOS::DistributedData;
namespace OHOS::UDMF {
using namespace testing::ext;
class UtdServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: OnRemoteRequest001
* @tc.desc: Abnormal test of OnRemoteRequest, code is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceStubTest, OnRemoteRequest001, TestSize.Level1)
{
    uint32_t code = 170; // invalid code
    MessageParcel data;
    MessageParcel reply;
    UtdServiceImpl utdServiceImpl;
    int ret = utdServiceImpl.OnRemoteRequest(code, data, reply);
    EXPECT_EQ(ret, E_IPC);
}

/**
* @tc.name: OnRegServiceNotifier001
* @tc.desc: Abnormal test of OnRegServiceNotifier
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceStubTest, OnRegServiceNotifier001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UtdServiceImpl utdServiceImpl;
    auto ret = utdServiceImpl.OnRegServiceNotifier(data, reply);
    EXPECT_EQ(ret, E_ERROR);
}

/**
* @tc.name: OnRegisterTypeDescriptors001
* @tc.desc: Abnormal test of OnRegisterTypeDescriptors, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceStubTest, OnRegisterTypeDescriptors001, TestSize.Level1)
{
    MessageParcel data;
    std::vector<TypeDescriptorCfg> descriptors;
    auto status = ITypesUtil::Marshal(data, descriptors);
    EXPECT_TRUE(status);
    MessageParcel reply;
    UtdServiceImpl utdServiceImpl;
    auto ret = utdServiceImpl.OnRegisterTypeDescriptors(data, reply);
    EXPECT_EQ(ret, E_OK);
    status = ITypesUtil::Unmarshal(reply, ret);
    EXPECT_TRUE(status);
    EXPECT_EQ(ret, E_NO_SYSTEM_PERMISSION);
}

/**
* @tc.name: OnUnregisterTypeDescriptors001
* @tc.desc: Abnormal test of OnUnregisterTypeDescriptors
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceStubTest, OnUnregisterTypeDescriptors001, TestSize.Level1)
{
    MessageParcel data;
    std::vector<std::string> typeIds;
    auto status = ITypesUtil::Marshal(data, typeIds);
    EXPECT_TRUE(status);
    MessageParcel reply;
    UtdServiceImpl utdServiceImpl;
    auto ret = utdServiceImpl.OnUnregisterTypeDescriptors(data, reply);
    EXPECT_EQ(ret, E_OK);
    status = ITypesUtil::Unmarshal(reply, ret);
    EXPECT_TRUE(status);
    EXPECT_EQ(ret, E_NO_SYSTEM_PERMISSION);
}
}; // namespace UDMF
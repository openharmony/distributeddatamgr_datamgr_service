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

#define LOG_TAG "UdmfServiceStubTest"
#include "udmf_service_stub.h"
#include "udmf_service_impl.h"
#include "udmf_types_util.h"
#include "gtest/gtest.h"

using namespace OHOS::DistributedData;
namespace OHOS::UDMF {
using namespace testing::ext;
class UdmfServiceStubTest : public testing::Test {
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
HWTEST_F(UdmfServiceStubTest, OnRemoteRequest001, TestSize.Level1)
{
    uint32_t code = 17; // invalid code
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnRemoteRequest(code, data, reply);
    EXPECT_EQ(ret, -1);
}

/**
* @tc.name: OnSetData001
* @tc.desc: Abnormal test of OnSetData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnSetData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnSetData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnGetData001
* @tc.desc: Abnormal test of OnGetData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnGetData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnGetData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnGetBatchData001
* @tc.desc: Abnormal test of OnGetBatchData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnGetBatchData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnGetBatchData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnUpdateData001
* @tc.desc: Abnormal test of OnUpdateData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnUpdateData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnUpdateData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnDeleteData001
* @tc.desc: Abnormal test of OnDeleteData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnDeleteData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnDeleteData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnGetSummary001
* @tc.desc: Abnormal test of OnGetSummary, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnGetSummary001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnGetSummary(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnAddPrivilege001
* @tc.desc: Abnormal test of OnAddPrivilege, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnAddPrivilege001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnAddPrivilege(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnIsRemoteData001
* @tc.desc: Abnormal test of OnIsRemoteData, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnIsRemoteData001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnIsRemoteData(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnSetAppShareOption001
* @tc.desc: Abnormal test of OnSetAppShareOption, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnSetAppShareOption001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnSetAppShareOption(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnGetAppShareOption001
* @tc.desc: Abnormal test of OnGetAppShareOption, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnGetAppShareOption001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnGetAppShareOption(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name: OnRemoveAppShareOption001
* @tc.desc: Abnormal test of OnRemoveAppShareOption, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnRemoveAppShareOption001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnRemoveAppShareOption(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name:OnObtainAsynProcess001
* @tc.desc: Abnormal test of OnObtainAsynProcess, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnObtainAsynProcess001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnObtainAsynProcess(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}

/**
* @tc.name:OnClearAsynProcessByKey001
* @tc.desc: Abnormal test of OnClearAsynProcessByKey, data is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceStubTest, OnClearAsynProcessByKey001, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    UdmfServiceImpl udmfServiceImpl;
    int ret = udmfServiceImpl.OnClearAsynProcessByKey(data, reply);
    EXPECT_EQ(ret, E_READ_PARCEL_ERROR);
}
}; // namespace UDMF
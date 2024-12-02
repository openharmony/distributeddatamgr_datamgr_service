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

#define LOG_TAG "UdmfServiceImplTest"
#include "udmf_service_impl.h"
#include "gtest/gtest.h"
#include "error_code.h"
#include "text.h"

using namespace OHOS::DistributedData;
namespace OHOS::UDMF {
using namespace testing::ext;
class UdmfServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: SaveData001
* @tc.desc: Abnormal test of SaveData, unifiedData is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SaveData001, TestSize.Level1)
{
    CustomOption option;
    UnifiedData data;
    std::string key = "";
    option.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SaveData(option, data, key);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: RetrieveData001
* @tc.desc: Abnormal test of RetrieveData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, RetrieveData001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData data;
    query.intention = UD_INTENTION_BASE;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.RetrieveData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsPermissionInCache002
* @tc.desc: Abnormal test of IsPermissionInCache, privilegeCache_ has no query.key
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsPermissionInCache002, TestSize.Level1)
{
    QueryOption query;
    UdmfServiceImpl udmfServiceImpl;
    bool ret = udmfServiceImpl.IsPermissionInCache(query);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: UpdateData001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, UpdateData001, TestSize.Level1)
{
    QueryOption query;
    UnifiedData data;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.UpdateData(query, data);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: GetSummary001
* @tc.desc: Abnormal test of UpdateData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, GetSummary001, TestSize.Level1)
{
    QueryOption query;
    Summary summary;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.GetSummary(query, summary);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: Sync001
* @tc.desc: Abnormal test of Sync, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, Sync001, TestSize.Level1)
{
    QueryOption query;
    std::vector<std::string> devices = {"device1"};
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.Sync(query, devices);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: IsRemoteData001
* @tc.desc: Abnormal test of IsRemoteData, query.key is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, IsRemoteData001, TestSize.Level1)
{
    QueryOption query;
    bool result = false;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.IsRemoteData(query, result);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption001
* @tc.desc: Abnormal test of SetAppShareOption, intention is empty
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption001, TestSize.Level1)
{
    std::string intention = "";
    int32_t shareOption = 1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}


/**
* @tc.name: SetAppShareOption002
* @tc.desc: Abnormal test of SetAppShareOption, shareOption > SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption002, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = 4;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption003
* @tc.desc: Abnormal test of SetAppShareOption, shareOption = SHARE_OPTIONS_BUTT
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption003, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = 3;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}

/**
* @tc.name: SetAppShareOption004
* @tc.desc: Abnormal test of SetAppShareOption, shareOption < IN_APP
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfServiceImplTest, SetAppShareOption004, TestSize.Level1)
{
    std::string intention = "intention";
    int32_t shareOption = -1;
    UdmfServiceImpl udmfServiceImpl;
    int32_t ret = udmfServiceImpl.SetAppShareOption(intention, shareOption);
    EXPECT_EQ(ret, E_INVALID_PARAMETERS);
}
}; // namespace UDMF
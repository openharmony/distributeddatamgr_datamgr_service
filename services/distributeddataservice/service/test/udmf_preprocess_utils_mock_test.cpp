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
#include <gtest/gtest.h>

#include "access_token_mock.h"
#include "ipc_skeleton.h"
#include "preprocess_utils.h"

namespace OHOS::UDMF {
using namespace testing;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Security::AccessToken;
class UdmfPreProcessUtilsMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
};

void UdmfPreProcessUtilsMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
}

void UdmfPreProcessUtilsMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
}

/**
* @tc.name: GetHapUidByToken001
* @tc.desc: Abnormal test of GetHapUidByToken, AccessTokenKit GetHapTokenInfo failed
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetHapUidByToken001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int userId = 0;
    PreProcessUtils preProcessUtils;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));
    int32_t ret = preProcessUtils.GetHapUidByToken(tokenId, userId);
    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: GetInstIndex001
* @tc.desc: Abnormal test of GetInstIndex, AccessTokenKit GetInstIndex failed
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetInstIndex001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int32_t instIndex = 0;
    PreProcessUtils preProcessUtils;
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_HAP));
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));
    bool ret = preProcessUtils.GetInstIndex(tokenId, instIndex);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: GetAlterableBundleNameByTokenId001
* @tc.desc: Abnormal test of GetSpecificBundleNameByTokenId
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetAlterableBundleNameByTokenId001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_SHELL));
    std::string bundleName = "";
    std::string specificBundleName = "";
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.GetSpecificBundleNameByTokenId(tokenId, specificBundleName, bundleName);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: GetAlterableBundleNameByTokenId002
* @tc.desc: Normal test of GetSpecificBundleNameByTokenId for native
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetAlterableBundleNameByTokenId002, TestSize.Level1)
{
    uint32_t tokenId = 999;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_NATIVE));
    EXPECT_CALL(*accessTokenKitMock, GetNativeTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));
    std::string bundleName = "";
    std::string specificBundleName = "";
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.GetSpecificBundleNameByTokenId(tokenId, specificBundleName, bundleName);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: GetAlterableBundleNameByTokenId003
* @tc.desc: Normal test of GetSpecificBundleNameByTokenId for hap
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetAlterableBundleNameByTokenId003, TestSize.Level1)
{
    uint32_t tokenId = 9999;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));
    std::string bundleName = "";
    std::string specificBundleName = "";
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.GetSpecificBundleNameByTokenId(tokenId, specificBundleName, bundleName);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: GetAlterableBundleNameByTokenId004
* @tc.desc: Abnormal test of GetSpecificBundleNameByTokenId for native
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetAlterableBundleNameByTokenId004, TestSize.Level1)
{
    uint32_t tokenId = 999;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_NATIVE));
    EXPECT_CALL(*accessTokenKitMock, GetNativeTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    std::string bundleName = "";
    std::string specificBundleName = "";
    PreProcessUtils preProcessUtils;
    bool ret = preProcessUtils.GetSpecificBundleNameByTokenId(tokenId, specificBundleName, bundleName);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: FillRuntimeInfo001
* @tc.desc: Normal test of FillRuntimeInfo
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, FillRuntimeInfo001, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_NATIVE));
    EXPECT_CALL(*accessTokenKitMock, GetNativeTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));

    UnifiedData data;
    CustomOption option;
    option.intention = UD_INTENTION_DATA_HUB;
    option.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    DataLoadInfo dataLoadInfo;
    PreProcessUtils preProcessUtils;
    auto ret = preProcessUtils.FillRuntimeInfo(data, option, dataLoadInfo, false);
    EXPECT_EQ(ret, E_OK);
}
}; // namespace UDMF
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
#include "unified_html_record_process.h"
#include "unified_record.h"

namespace OHOS::UDMF {
using namespace testing;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Security::AccessToken;

namespace {
constexpr uint32_t TEST_TOKEN_ID = 9999;
constexpr int32_t TEST_USER_ID = 100;
constexpr const char *TEST_BUNDLE_NAME = "ohos.test.udmf.preprocess";

std::shared_ptr<UnifiedRecord> CreateHtmlRecord(const std::string &uri)
{
    std::string html = "<img data-ohos='clipboard' src='" + uri + "'>";
    auto obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.html";
    obj->value_["htmlContent"] = html;
    obj->value_["plainContent"] = "";
    auto record = std::make_shared<UnifiedRecord>(UDType::HTML, obj);
    UnifiedHtmlRecordProcess::GetUriFromHtmlRecord(*record);
    return record;
}

std::vector<UriInfo> CollectUris(const std::shared_ptr<UnifiedRecord> &record)
{
    std::vector<UriInfo> uris;
    record->ComputeUris([&uris] (UriInfo &uriInfo) {
        uris.push_back(uriInfo);
        return true;
    });
    return uris;
}

int FillHapTokenInfo(AccessTokenID, HapTokenInfo &hapInfo)
{
    hapInfo.bundleName = TEST_BUNDLE_NAME;
    hapInfo.userID = TEST_USER_ID;
    return RET_SUCCESS;
}
} // namespace

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
    auto ret = preProcessUtils.FillRuntimeInfo(data, option);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(data.GetRuntime()->permissionPolicyMode, PERMISSION_POLICY_MODE_MASK);
}

/**
* @tc.name: FillDelayRuntimeInfo001
* @tc.desc: Normal test of FillDelayRuntimeInfo
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, FillDelayRuntimeInfo001, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Return(RET_FAILED));
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_NATIVE));
    EXPECT_CALL(*accessTokenKitMock, GetNativeTokenInfo(_, _)).WillRepeatedly(Return(RET_SUCCESS));

    UnifiedData data;
    CustomOption option;
    option.intention = UD_INTENTION_DATA_HUB;
    option.tokenId = static_cast<uint32_t>(IPCSkeleton::GetCallingTokenID());
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.sequenceKey = "123";
    dataLoadInfo.recordCount = 10;
    PreProcessUtils preProcessUtils;
    auto ret = preProcessUtils.FillDelayRuntimeInfo(data, option, dataLoadInfo);
    EXPECT_EQ(ret, E_OK);
    auto runtime = data.GetRuntime();
    EXPECT_EQ(runtime->recordTotalNum, dataLoadInfo.recordCount);
    EXPECT_EQ(runtime->dataStatus, DataStatus::WAITING);
    EXPECT_EQ(runtime->permissionPolicyMode, PERMISSION_POLICY_MODE_MASK);
}

/**
* @tc.name: ProcessHtmlRecord_PathTraversalUri_RejectsTraversalSegment
* @tc.desc: Reject an HTML image URI containing a literal parent-directory segment
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_PathTraversalUri_RejectsTraversalSegment, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::string oriUri = "file:///data/storage/el2/base/../victim.bundle/haps/image.png";
    auto record = CreateHtmlRecord(oriUri);
    auto parsedUris = CollectUris(record);
    ASSERT_EQ(parsedUris.size(), 1U);
    EXPECT_EQ(parsedUris.front().oriUri, oriUri);
    std::vector<std::string> uris;
    std::unordered_map<std::string, std::string> uriCache;

    PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

    auto processedUris = CollectUris(record);
    ASSERT_EQ(processedUris.size(), 1U);
    EXPECT_TRUE(processedUris.front().authUri.empty());
    EXPECT_TRUE(uris.empty());
}

/**
* @tc.name: ProcessHtmlRecord_EncodedPathTraversalUri_RejectsTraversalSegment
* @tc.desc: Reject encoded parent-directory segments and encoded path separators in HTML image URIs
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_EncodedPathTraversalUri_RejectsTraversalSegment,
    TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::vector<std::string> oriUris = {
        "file:///data/storage/el2/base/%2e%2e/victim.bundle/haps/image.png",
        "file:///data/storage/el2/base/%2E%2E/victim.bundle/haps/image.png",
        "file:///data/storage/el2/base%2f%2e%2e%2fvictim.bundle/haps/image.png",
        "file:///docs/%2e%2e/victim/image.png"
    };

    for (const auto &oriUri : oriUris) {
        auto record = CreateHtmlRecord(oriUri);
        auto parsedUris = CollectUris(record);
        ASSERT_EQ(parsedUris.size(), 1U);
        EXPECT_EQ(parsedUris.front().oriUri, oriUri);
        std::vector<std::string> uris;
        std::unordered_map<std::string, std::string> uriCache;
        PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

        auto processedUris = CollectUris(record);
        ASSERT_EQ(processedUris.size(), 1U);
        EXPECT_TRUE(processedUris.front().authUri.empty());
        EXPECT_TRUE(uris.empty());
    }
}

/**
* @tc.name: ProcessHtmlRecord_NullByteEncodedUri_RejectsUri
* @tc.desc: Reject an HTML image URI containing a URL-encoded null byte
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_NullByteEncodedUri_RejectsUri, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::string oriUri = "file:///data/storage/el2/base/haps/image%00.png";
    auto record = CreateHtmlRecord(oriUri);
    auto parsedUris = CollectUris(record);
    ASSERT_EQ(parsedUris.size(), 1U);
    EXPECT_EQ(parsedUris.front().oriUri, oriUri);
    std::vector<std::string> uris;
    std::unordered_map<std::string, std::string> uriCache;

    PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

    auto processedUris = CollectUris(record);
    ASSERT_EQ(processedUris.size(), 1U);
    EXPECT_TRUE(processedUris.front().authUri.empty());
    EXPECT_TRUE(uris.empty());
}

/**
* @tc.name: ProcessHtmlRecord_QuerySuffix_RejectsUri
* @tc.desc: Reject an HTML image URI whose apparent image extension is followed by a query
* @tc.type: FUNC
* @tc.author: agent
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_QuerySuffix_RejectsUri, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::string oriUri = "file:///data/storage/el2/base/haps/image.png?next=/other.jpg";
    auto record = CreateHtmlRecord(oriUri);
    std::vector<std::string> uris;
    std::unordered_map<std::string, std::string> uriCache;

    PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

    auto processedUris = CollectUris(record);
    ASSERT_EQ(processedUris.size(), 1U);
    EXPECT_TRUE(processedUris.front().authUri.empty());
    EXPECT_TRUE(uris.empty());
}

/**
* @tc.name: ProcessHtmlRecord_PathTraversalHtml_RejectsParsedUri
* @tc.desc: Parse traversal URIs from HTML and reject them during HTML record preprocessing
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_PathTraversalHtml_RejectsParsedUri, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::vector<std::string> oriUris = {
        "file:///data/storage/el2/base/../victim.bundle/haps/image.png",
        "file:///data/storage/el2/base/%2e%2e/victim.bundle/haps/image.png"
    };

    for (const auto &oriUri : oriUris) {
        auto record = CreateHtmlRecord(oriUri);
        auto parsedUris = CollectUris(record);
        ASSERT_EQ(parsedUris.size(), 1U);
        EXPECT_EQ(parsedUris.front().oriUri, oriUri);

        std::vector<std::string> uris;
        std::unordered_map<std::string, std::string> uriCache;
        PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

        auto processedUris = CollectUris(record);
        ASSERT_EQ(processedUris.size(), 1U);
        EXPECT_TRUE(processedUris.front().authUri.empty());
        EXPECT_TRUE(uris.empty());
    }
}

/**
* @tc.name: BuildHtmlAuthUri_DoubleDotInFileName_KeepsValidUri
* @tc.desc: Keep an HTML image URI when double dots are part of a regular file name
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, BuildHtmlAuthUri_DoubleDotInFileName_KeepsValidUri, TestSize.Level1)
{
    std::string oriUri = "file:///data/storage/el2/base/haps/file..png";
    EXPECT_EQ(PreProcessUtils::BuildHtmlAuthUri(oriUri, "ohos.test.udmf.preprocess"),
        "file://ohos.test.udmf.preprocess/data/storage/el2/base/haps/file..png");
}

/**
* @tc.name: BuildHtmlAuthUri_TwoCharacterPathSegment_KeepsValidUri
* @tc.desc: Keep a valid URI when a two-character path segment is not a parent-directory segment
* @tc.type: FUNC
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, BuildHtmlAuthUri_TwoCharacterPathSegment_KeepsValidUri, TestSize.Level1)
{
    std::string oriUri = "file:///data/storage/el2/base/ab/image.png";
    EXPECT_EQ(PreProcessUtils::BuildHtmlAuthUri(oriUri, "ohos.test.udmf.preprocess"),
        "file://ohos.test.udmf.preprocess/data/storage/el2/base/ab/image.png");
}

/**
* @tc.name: ProcessHtmlRecord_ClientUriMismatch_DoesNotAuthorizeOrOverwrite
* @tc.desc: Reject an HTML URI missing from the client-validated set and preserve prior HTML URI results
* @tc.type: FUNC
* @tc.author: agent
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_ClientUriMismatch_DoesNotAuthorizeOrOverwrite,
    TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    auto record = CreateHtmlRecord("file:///data/storage/el2/base/haps/image.png");
    std::string clientUri = "file:///data/storage/el2/base/haps/another.png";
    std::vector<std::string> uris = { "file://ohos.test.udmf.preprocess/already-added.png" };
    std::unordered_map<std::string, std::string> uriCache;

    PreProcessUtils::ProcessHtmlRecord(record, {clientUri}, TEST_TOKEN_ID, uris, uriCache);

    ASSERT_EQ(uris.size(), 1U);
    EXPECT_EQ(uris.front(), "file://ohos.test.udmf.preprocess/already-added.png");
    auto processedUris = CollectUris(record);
    ASSERT_EQ(processedUris.size(), 1U);
    EXPECT_TRUE(processedUris.front().authUri.empty());
}

/**
* @tc.name: ProcessHtmlRecord_CachedUri_ReusesValidationAcrossRecords
* @tc.desc: Reuse an authorized URI cached while processing an earlier HTML record
* @tc.type: FUNC
* @tc.author: agent
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, ProcessHtmlRecord_CachedUri_ReusesValidationAcrossRecords, TestSize.Level1)
{
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillRepeatedly(Invoke(FillHapTokenInfo));
    std::string oriUri = "file:///data/storage/el2/base/haps/image.png";
    std::string authUri = "file://ohos.test.udmf.preprocess/data/storage/el2/base/haps/image.png";
    auto record = CreateHtmlRecord(oriUri);
    std::vector<std::string> uris;
    std::unordered_map<std::string, std::string> uriCache = { { oriUri, authUri } };

    PreProcessUtils::ProcessHtmlRecord(record, {oriUri}, TEST_TOKEN_ID, uris, uriCache);

    ASSERT_EQ(uris.size(), 1U);
    EXPECT_EQ(uris.front(), authUri);
    auto processedUris = CollectUris(record);
    ASSERT_EQ(processedUris.size(), 1U);
    EXPECT_EQ(processedUris.front().authUri, authUri);
}
}; // namespace UDMF

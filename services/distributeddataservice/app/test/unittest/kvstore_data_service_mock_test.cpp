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
#include "kvstore_data_service.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace Security::AccessToken;
namespace OHOS::Test {
static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
class KvStoreDataServiceMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
void KvStoreDataServiceMockTest::SetUpTestCase(void)
{
}

void KvStoreDataServiceMockTest::TearDownTestCase(void)
{
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
}

void KvStoreDataServiceMockTest::SetUp(void)
{}

void KvStoreDataServiceMockTest::TearDown(void)
{}

/**
 * @tc.name: GetSelfBundleName_001
 * @tc.desc: Test GetSelfBundleName with get hap info success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(KvStoreDataServiceMockTest, GetSelfBundleName_001, TestSize.Level0)
{
    KvStoreDataService KvStoreDataServiceTest;
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillOnce(testing::Return(0))
        .WillRepeatedly(testing::Return(0));
    std::pair<int32_t, std::string>result = KvStoreDataServiceTest.GetSelfBundleName();
    EXPECT_EQ(result.first, Status::SUCCESS);
}

/**
 * @tc.name: GetSelfBundleName_002
 * @tc.desc: Test GetSelfBundleName with get hap info failed and get process success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(KvStoreDataServiceMockTest, GetSelfBundleName_002, TestSize.Level0)
{
    KvStoreDataService KvStoreDataServiceTest;
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillOnce(testing::Return(-1))
        .WillRepeatedly(testing::Return(-1));
    EXPECT_CALL(*accTokenMock, GetNativeTokenInfo(testing::_, testing::_))
        .WillOnce(testing::Return(0))
        .WillRepeatedly(testing::Return(0));
    std::pair<int32_t, std::string>result = KvStoreDataServiceTest.GetSelfBundleName();
    EXPECT_EQ(result.first, Status::SUCCESS);
}

/**
 * @tc.name: GetSelfBundleName_003
 * @tc.desc: Test GetSelfBundleName with get hap info failed and get process failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(KvStoreDataServiceMockTest, GetSelfBundleName_003, TestSize.Level0)
{
    KvStoreDataService KvStoreDataServiceTest;
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillOnce(testing::Return(-1))
        .WillRepeatedly(testing::Return(-1));
    EXPECT_CALL(*accTokenMock, GetNativeTokenInfo(testing::_, testing::_))
        .WillOnce(testing::Return(-1))
        .WillRepeatedly(testing::Return(-1));
    std::pair<int32_t, std::string>result = KvStoreDataServiceTest.GetSelfBundleName();
    EXPECT_EQ(result.first, Status::ERROR);
}
}
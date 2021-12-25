/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <vector>

#include "distributeddb_tools_unit_test.h"
#include "data_compression.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
#ifndef OMIT_ZLIB
// LENGTH IS 680.
unsigned char g_srcStr[] =
    "I come from Alabama with my banjo on my knee,"
    "I'm going to Louisiana, my true love for to see,"
    "It rained all night the day I left,"
    "The weather it was dry,"
    "The sun so hot, I froze to death,"
    "Susanna don't you cry,"
    "Oh, Susanna,"
    "Oh don't you cry for me,"
    "For I come from Alabama,"
    "With my banjo on my knee,"
    "I had a dream the other night when everything was still,"
    "I thought I saw Susanna a-coming down the hill,"
    "The buckwheat cake was in her mouth,"
    "The tear was in her eye,"
    "Says I, I'm coming from the south,"
    "Susanna, don't you cry,"
    "Oh, Susanna,"
    "Oh don't you cry for me,"
    "I'm going to Louisiana,"
    "With my banjo on my knee,"
    "Oh, Susanna,"
    "Oh don't you cry for me,"
    "I'm going to Louisiana,"
    "With my banjo on my knee.";
}
#endif
class DistributedDBDataCompressionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBDataCompressionTest::SetUpTestCase(void)
{}

void DistributedDBDataCompressionTest::TearDownTestCase(void)
{}

void DistributedDBDataCompressionTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBDataCompressionTest::TearDown(void)
{}

/**
  * @tc.name: DataCompression1
  * @tc.desc: To test the function compress and uncompress works well in normal situation.
  * @tc.type: FUNC
  * @tc.require: AR000G3QTT
  * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBDataCompressionTest, DataCompression1, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare a source data. And compress it.
     * @tc.expected: step1. Compress successfully. Compressed data length is less than srcLen.
     */
#ifndef OMIT_ZLIB
    const int origLen = sizeof(g_srcStr);
    vector<uint8_t> srcData(g_srcStr, g_srcStr + sizeof(g_srcStr));

    vector<uint8_t> compressedData;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Compress(srcData, compressedData), E_OK);
    EXPECT_LT(compressedData.size(), srcData.size());

    /**
     * @tc.steps:step2. Uncompress the compressed data.
     * @tc.expected: step2. Uncompress successfully. Uncompressed data equals to source data.
     */
    vector<uint8_t> uncompressedData;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Uncompress(
        compressedData, uncompressedData, origLen), E_OK);
    EXPECT_EQ(srcData, uncompressedData);
#endif // OMIT_ZLIB
}

/**
  * @tc.name: DataCompression2
  * @tc.desc: To test uncompress failed when compressed is destroyed.
  * @tc.type: FUNC
  * @tc.require: AR000G3QTT
  * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBDataCompressionTest, DataCompression2, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare a source data. And compress it.
     * @tc.expected: step1. Compress successfully. Compressed data length is less than srcLen.
     */
#ifndef OMIT_ZLIB
    const int origLen = sizeof(g_srcStr);
    vector<uint8_t> srcData(g_srcStr, g_srcStr + sizeof(g_srcStr));

    vector<uint8_t> compressedData;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Compress(srcData, compressedData), E_OK);
    EXPECT_LT(compressedData.size(), srcData.size());

    /**
     * @tc.steps:step2. Destroy the compressed data.
     */
    *(compressedData.begin()) = ~*(compressedData.begin());

    /**
     * @tc.steps:step3. Uncompress the compressed data.
     * @tc.expected: step3. Uncompressed failed and return -E_SYSTEM_API_FAIL.
     */
    vector<uint8_t> uncompressedData;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Uncompress(
        compressedData, uncompressedData, origLen), -E_SYSTEM_API_FAIL);
#endif // OMIT_ZLIB
}

/**
  * @tc.name: DataCompression3
  * @tc.desc: To test uncompress works when bufferLen is larger but under 30M limit,
              and uncompress failed when bufferLen is beyond the 30M limit.
  * @tc.type: FUNC
  * @tc.require: AR000G3QTT
  * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBDataCompressionTest, DataCompression3, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare a source data. And compress it.
     * @tc.expected: step1. Compress successfully. Compressed data length is less than srcLen.
     */
#ifndef OMIT_ZLIB
    vector<uint8_t> srcData(g_srcStr, g_srcStr + sizeof(g_srcStr));

    vector<uint8_t> compressedData;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Compress(srcData, compressedData), E_OK);
    EXPECT_LT(compressedData.size(), srcData.size());

    /**
     * @tc.steps:step2. Set origLen a larger num under 30M limit. And uncompress.
     * @tc.expected: step1. Uncompress successfully.
     */
    vector<uint8_t> uncompressedData;
    int incorrectLen = 10000;
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Uncompress(
        compressedData, uncompressedData, incorrectLen), E_OK);
    EXPECT_EQ(srcData, uncompressedData);

    /**
     * @tc.steps:step2. Set origLen a larger num beyond 30M limit. And uncompress.
     * @tc.expected: step1. Uncompress failed and return E_INVALID_ARGS.
     */
    uncompressedData.clear();
    incorrectLen = 31457281; // 30M + 1
    EXPECT_EQ(DataCompression::GetInstance(CompressAlgorithm::ZLIB)->Uncompress(
        compressedData, uncompressedData, incorrectLen), -E_INVALID_ARGS);
#endif // OMIT_ZLIB
}

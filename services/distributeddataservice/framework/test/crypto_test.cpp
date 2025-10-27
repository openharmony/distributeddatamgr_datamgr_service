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
#include "utils/crypto.h"
#include "error/general_error.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace std;

class CryptoTest : public testing::Test {
};

/**
* @tc.name: RandomTest
* @tc.desc: Random.
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(CryptoTest, RandomTest, TestSize.Level1)
{
    int32_t len = 10;
    int32_t min = 0;
    int32_t max = 255;

    vector<uint8_t> randomNumbers = Crypto::Random(len, min, max);
    ASSERT_EQ(randomNumbers.size(), 10);
}

/**
 * @tc.name: Crypto_Sha256_InvalidInput_ReturnsEmpty
 * @tc.desc: Verify Sha256 returns empty string for invalid inputs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_InvalidInput_ReturnsEmpty, TestSize.Level1)
{
    // Test case 1: nullptr with zero size
    auto result = Crypto::Sha256(nullptr, 0);
    EXPECT_TRUE(result.empty()) << "Should return empty for nullptr with zero size";
}

/**
 * @tc.name: Crypto_Sha256_ValidInput_ReturnsCorrectHash
 * @tc.desc: Verify Sha256 returns correct hash for valid inputs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_ValidInput_ReturnsCorrectHash, TestSize.Level1)
{
    // Test case 1: Basic string
    std::string testData = "hello world";
    std::string result = Crypto::Sha256(testData.data(), testData.size());

    // SHA256 of "hello world" is known value
    std::string expected = "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9";
    EXPECT_EQ(result, expected) << "SHA256 hash mismatch for 'hello world'";
}

/**
 * @tc.name: Crypto_Sha256_CaseSensitivity_WorksCorrectly
 * @tc.desc: Verify Sha256 case sensitivity parameter works correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_CaseSensitivity_WorksCorrectly, TestSize.Level1)
{
    std::string testData = "test";

    // Test case 1: Lowercase output
    std::string lowerResult = Crypto::Sha256(testData.data(), testData.size(), false);
    EXPECT_FALSE(lowerResult.empty());

    // Verify all characters are lowercase hex
    for (char c : lowerResult) {
        EXPECT_TRUE(std::islower(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)))
            << "Lowercase output should contain only lowercase hex digits";
    }

    // Test case 2: Uppercase output
    std::string upperResult = Crypto::Sha256(testData.data(), testData.size(), true);
    EXPECT_FALSE(upperResult.empty());

    // Verify all characters are uppercase hex
    for (char c : upperResult) {
        EXPECT_TRUE(std::isupper(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)))
            << "Uppercase output should contain only uppercase hex digits";
    }

    // Test case 3: Case difference
    // The hash values should be the same except for case
    for (size_t i = 0; i < lowerResult.size(); ++i) {
        if (std::isalpha(static_cast<unsigned char>(lowerResult[i]))) {
            EXPECT_EQ(std::toupper(lowerResult[i]), upperResult[i]) << "Characters should differ only in case";
        } else {
            EXPECT_EQ(lowerResult[i], upperResult[i]) << "Digit characters should be identical";
        }
    }
}

/**
 * @tc.name: Crypto_Sha256_BinaryData_WorksCorrectly
 * @tc.desc: Verify Sha256 works correctly with binary data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_BinaryData_WorksCorrectly, TestSize.Level1)
{
    // Test case 1: Binary data with null bytes
    std::vector<uint8_t> binaryData = { 0x00, 0x01, 0x02, 0x03, 0x00, 0xFF };
    std::string result = Crypto::Sha256(binaryData.data(), binaryData.size());

    EXPECT_FALSE(result.empty()) << "SHA256 of binary data should not be empty";
    EXPECT_EQ(result.length(), 64) << "SHA256 hash should be 64 characters (256 bits in hex)";

    // Test case 2: Single byte
    uint8_t singleByte = 0x42;
    result = Crypto::Sha256(&singleByte, sizeof(singleByte));
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.length(), 64);
}

/**
 * @tc.name: Crypto_Sha256_Consistency_ReturnsSameHash
 * @tc.desc: Verify Sha256 returns consistent results for same input
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_Consistency_ReturnsSameHash, TestSize.Level1)
{
    std::string testData = "consistent test data";

    // Multiple calls with same input should produce same output
    std::string firstResult = Crypto::Sha256(testData.data(), testData.size());
    std::string secondResult = Crypto::Sha256(testData.data(), testData.size());
    std::string thirdResult = Crypto::Sha256(testData.data(), testData.size());

    EXPECT_EQ(firstResult, secondResult) << "Multiple calls should return identical hash";
    EXPECT_EQ(secondResult, thirdResult) << "Multiple calls should return identical hash";
    EXPECT_EQ(firstResult, thirdResult) << "Multiple calls should return identical hash";
}

/**
 * @tc.name: Crypto_Sha256_DifferentInput_ReturnsDifferentHash
 * @tc.desc: Verify Sha256 returns different hashes for different inputs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_DifferentInput_ReturnsDifferentHash, TestSize.Level1)
{
    // Test case 1: Slightly different inputs
    std::string data1 = "hello world";
    std::string data2 = "hello world!"; // One character difference

    std::string hash1 = Crypto::Sha256(data1.data(), data1.size());
    std::string hash2 = Crypto::Sha256(data2.data(), data2.size());

    EXPECT_NE(hash1, hash2) << "Different inputs should produce different hashes";

    // Test case 2: Completely different inputs
    std::string data3 = "completely different";
    std::string hash3 = Crypto::Sha256(data3.data(), data3.size());

    EXPECT_NE(hash1, hash3) << "Completely different inputs should produce different hashes";
    EXPECT_NE(hash2, hash3) << "Completely different inputs should produce different hashes";
}

/**
 * @tc.name: Crypto_Sha256_LargeInput_WorksCorrectly
 * @tc.desc: Verify Sha256 works correctly with large inputs
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_LargeInput_WorksCorrectly, TestSize.Level1)
{
    // Test case 1: 1KB data
    std::vector<uint8_t> largeData(1024, 0x41); // 1KB of 'A'
    std::string result = Crypto::Sha256(largeData.data(), largeData.size());

    EXPECT_FALSE(result.empty()) << "SHA256 of large data should not be empty";
    EXPECT_EQ(result.length(), 64) << "SHA256 hash should always be 64 characters";
}

/**
 * @tc.name: Crypto_Sha256_EdgeCases_HandlesCorrectly
 * @tc.desc: Verify Sha256 handles edge cases correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_EdgeCases_HandlesCorrectly, TestSize.Level1)
{
    // Test case 1: Minimum valid input (1 byte)
    uint8_t minData = 0x01;
    std::string result = Crypto::Sha256(&minData, 1);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.length(), 64);

    // Test case 2: Verify hash length is always 64 characters
    std::vector<std::string> testInputs = { "a", "ab", "abc", "abcd", "abcde",
        "This is a longer test string to verify consistent output length" };

    for (const auto &input : testInputs) {
        result = Crypto::Sha256(input.data(), input.size());
        EXPECT_EQ(result.length(), 64) << "SHA256 hash should always be 64 characters for input: " << input;
    }
}

/**
 * @tc.name: Crypto_Sha256_InputExactly4KB_ShouldSuccess
 * @tc.desc: Verify Sha256 handles exactly 4KB input correctly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_InputExactly4KB_ShouldSuccess, TestSize.Level1)
{
    std::vector<uint8_t> data(4 * 1024, 0x41); // 4KB of 'A'

    std::string result = Crypto::Sha256(data.data(), data.size(), true);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result, "6896D9EA3F73A4434F5832BC65714E7D066F177373F36F34DC8A6F735DAA41B1");
}

/**
 * @tc.name: Crypto_Sha256_InputExceeds4KB_ShouldReturnEmpty
 * @tc.desc: Verify Sha256 returns empty when input exceeds 4KB limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(CryptoTest, Sha256_InputExceeds4KB_ShouldReturnEmpty, TestSize.Level1)
{
    std::vector<uint8_t> data(4 * 1024 + 1, 0x42); // 4KB+1 of 'B'

    std::string result = Crypto::Sha256(data.data(), data.size(), false);

    EXPECT_TRUE(result.empty());
}
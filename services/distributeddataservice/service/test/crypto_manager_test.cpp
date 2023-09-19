/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "crypto_manager.h"

#include <random>
#include "gtest/gtest.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
class CryptoManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {};
    void TearDown() {};

protected:
    static std::vector<uint8_t> randomKey;
    static std::vector<uint8_t> Random(uint32_t len);
};

static const uint32_t KEY_LENGTH = 32;
static const uint32_t ENCRYPT_KEY_LENGTH = 48;
std::vector<uint8_t> CryptoManagerTest::randomKey;
void CryptoManagerTest::SetUpTestCase(void)
{
    randomKey = Random(KEY_LENGTH);
}

void CryptoManagerTest::TearDownTestCase(void)
{
    randomKey.assign(randomKey.size(), 0);
}

std::vector<uint8_t> CryptoManagerTest::Random(uint32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

/**
* @tc.name: GenerateRootKey
* @tc.desc: generate the root key
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, GenerateRootKey, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    EXPECT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);
}

/**
* @tc.name: CheckRootKey
* @tc.desc: check root key exist;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, CheckRootKey, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().CheckRootKey();
    EXPECT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);
}

/**
* @tc.name: Encrypt001
* @tc.desc: encrypt random key;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, Encrypt001, TestSize.Level0)
{
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey);
    EXPECT_EQ(encryptKey.size(), ENCRYPT_KEY_LENGTH);
    encryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: Encrypt002
* @tc.desc: encrypt empty key;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, Encrypt002, TestSize.Level0)
{
    auto encryptKey = CryptoManager::GetInstance().Encrypt({ });
    EXPECT_TRUE(encryptKey.empty());
}

/**
* @tc.name: DecryptKey001
* @tc.desc: decrypt the encrypt key;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, DecryptKey001, TestSize.Level0)
{
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey);
    std::vector<uint8_t> key;
    auto result = CryptoManager::GetInstance().Decrypt(encryptKey, key);
    EXPECT_TRUE(result);
    EXPECT_EQ(key.size(), KEY_LENGTH);
    encryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: DecryptKey002
* @tc.desc: decrypt the key, the source key is not encrypt;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, DecryptKey002, TestSize.Level0)
{
    std::vector<uint8_t> key;
    auto result = CryptoManager::GetInstance().Decrypt(randomKey, key);
    EXPECT_FALSE(result);
    EXPECT_TRUE(key.empty());
}

/**
* @tc.name: DecryptKey003
* @tc.desc: decrypt the key, the source key is empty;
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(CryptoManagerTest, DecryptKey003, TestSize.Level0)
{
    std::vector<uint8_t> srcKey {};
    std::vector<uint8_t> key;
    auto result = CryptoManager::GetInstance().Decrypt(srcKey, key);
    EXPECT_FALSE(result);
    EXPECT_TRUE(key.empty());
}
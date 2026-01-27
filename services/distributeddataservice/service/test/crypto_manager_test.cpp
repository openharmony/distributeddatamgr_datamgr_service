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

#include "crypto/crypto_manager.h"

#include <random>

#include "gtest/gtest.h"

#include "access_token.h"
#include "accesstoken_kit.h"
#include "hks_api.h"
#include "hks_param.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

static constexpr int32_t KEY_LENGTH = 32;
static constexpr int32_t NONCE_SIZE = 12;
static constexpr int32_t INVALID_AREA = -1;
static constexpr int32_t TEST_USERID_NUM = 100;
static constexpr const char *TEST_USERID = "100";
static constexpr const char *TEST_BUNDLE_NAME = "test_application";
static constexpr const char *TEST_STORE_NAME = "test_store";
static constexpr const char *ROOT_KEY_ALIAS = "distributed_db_root_key";
static constexpr const char *PROCESS_NAME = "distributeddata";

class CryptoManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

    static void SetNativeTokenIdFromProcess(const std::string &process);
    static std::vector<uint8_t> Random(int32_t len);
    static void DeleteRootKey(int32_t area);
    static uint32_t GetStorageLevel(int32_t area);

    static std::vector<uint8_t> randomKey;
    static std::vector<uint8_t> vecRootKeyAlias;
    static std::vector<uint8_t> nonce;
};

std::vector<uint8_t> CryptoManagerTest::randomKey;
std::vector<uint8_t> CryptoManagerTest::vecRootKeyAlias;
std::vector<uint8_t> CryptoManagerTest::nonce;

void CryptoManagerTest::SetNativeTokenIdFromProcess(const std::string &process)
{
    std::string dumpInfo;
    AtmToolsParamInfo info;
    info.processName = process;
    AccessTokenKit::DumpTokenInfo(info, dumpInfo);
    size_t pos = dumpInfo.find("\"tokenID\": ");
    if (pos == std::string::npos) {
        return;
    }
    pos += std::string("\"tokenID\": ").length();
    std::string numStr;
    while (pos < dumpInfo.length() && std::isdigit(dumpInfo[pos])) {
        numStr += dumpInfo[pos];
        ++pos;
    }
    std::istringstream iss(numStr);
    AccessTokenID tokenID;
    iss >> tokenID;
    SetSelfTokenID(tokenID);
}

void CryptoManagerTest::SetUpTestCase(void)
{
    SetNativeTokenIdFromProcess(PROCESS_NAME);

    randomKey = Random(KEY_LENGTH);
    vecRootKeyAlias = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    nonce = Random(NONCE_SIZE);
}

void CryptoManagerTest::TearDownTestCase(void)
{
    randomKey.assign(randomKey.size(), 0);
    nonce.assign(nonce.size(), 0);
    DeleteRootKey(CryptoManager::Area::EL1);
    DeleteRootKey(CryptoManager::Area::EL2);
    DeleteRootKey(CryptoManager::Area::EL4);
}

std::vector<uint8_t> CryptoManagerTest::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

uint32_t CryptoManagerTest::GetStorageLevel(int32_t area)
{
    if (area >= CryptoManager::Area::EL4 && area <= CryptoManager::Area::EL5) {
        return HKS_AUTH_STORAGE_LEVEL_ECE;
    }
    if (area >= CryptoManager::Area::EL2 && area <= CryptoManager::Area::EL3) {
        return HKS_AUTH_STORAGE_LEVEL_CE;
    }
    return HKS_AUTH_STORAGE_LEVEL_DE;
}

void CryptoManagerTest::DeleteRootKey(int32_t area)
{
    struct HksParamSet *params = nullptr;
    if (HksInitParamSet(&params) != HKS_SUCCESS) {
        return;
    }
    auto storageLevel = GetStorageLevel(area);
    std::vector<HksParam> hksParam = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = storageLevel },
    };
    if (storageLevel > HKS_AUTH_STORAGE_LEVEL_DE) {
        hksParam.emplace_back(HksParam { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = TEST_USERID_NUM });
    }
    if (HksAddParams(params, hksParam.data(), hksParam.size()) != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }
    if (HksBuildParamSet(&params) != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return;
    }
    struct HksBlob keyName = { uint32_t(vecRootKeyAlias.size()), const_cast<uint8_t *>(vecRootKeyAlias.data()) };
    (void)HksDeleteKey(&keyName, params);
    HksFreeParamSet(&params);
}

/**
* @tc.name: GenerateRootKeyTest001
* @tc.desc: generate the root key of DE
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, GenerateRootKeyTest001, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    EXPECT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    errCode = CryptoManager::GetInstance().CheckRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);
}

/**
* @tc.name: EncryptAndDecryptTest001
* @tc.desc: encrypt random key and decrypt with EL1
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest001, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL1, .userId = TEST_USERID };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = CryptoManager::Area::EL1, .userId = TEST_USERID,
        .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest002
* @tc.desc: encrypt random key and decrypt with EL2
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest002, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL2, .userId = TEST_USERID };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = CryptoManager::Area::EL2, .userId = TEST_USERID,
        .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest003
* @tc.desc: encrypt random key and decrypt with EL3
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest003, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL3, .userId = TEST_USERID };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = CryptoManager::Area::EL3, .userId = TEST_USERID,
        .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest004
* @tc.desc: encrypt random key and decrypt with EL4
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest004, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL4, .userId = TEST_USERID };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = CryptoManager::Area::EL4, .userId = TEST_USERID,
        .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest005
* @tc.desc: encrypt random key and decrypt with INVALID_AREA
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest005, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = INVALID_AREA, .userId = TEST_USERID };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = encryptParams.area, .userId = TEST_USERID,
        .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest006
* @tc.desc: encrypt random key and decrypt with EL1 and key alias
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest006, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL1, .userId = TEST_USERID,
        .keyAlias = vecRootKeyAlias };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    CryptoManager::CryptoParams decryptParams = { .area = encryptParams.area, .userId = TEST_USERID,
        .keyAlias = vecRootKeyAlias, .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: EncryptAndDecryptTest007
* @tc.desc: encrypt random key and decrypt with EL1 and nonce value
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, EncryptAndDecryptTest007, TestSize.Level0)
{
    CryptoManager::CryptoParams encryptParams = { .area = CryptoManager::Area::EL1, .userId = TEST_USERID,
        .nonce = nonce };
    auto encryptKey = CryptoManager::GetInstance().Encrypt(randomKey, encryptParams);
    ASSERT_FALSE(encryptKey.empty());

    CryptoManager::CryptoParams decryptParams = { .area = encryptParams.area, .userId = TEST_USERID,
        .keyAlias = vecRootKeyAlias, .nonce = encryptParams.nonce };
    auto decryptKey = CryptoManager::GetInstance().Decrypt(encryptKey, decryptParams);
    ASSERT_FALSE(decryptKey.empty());
    for (auto i = 0; i < KEY_LENGTH; ++i) {
        ASSERT_EQ(decryptKey[i], randomKey[i]);
    }
    decryptKey.assign(encryptKey.size(), 0);
}

/**
* @tc.name: UpdateSecretMetaTest001
* @tc.desc: update meta with invalid params
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest001, TestSize.Level0)
{
    std::vector<uint8_t> password;
    StoreMetaData metaData;
    SecretKeyMetaData secretKey;
    CryptoManager::GetInstance().UpdateSecretMeta(password, metaData, "", secretKey);
    ASSERT_TRUE(secretKey.sKey.empty());

    secretKey.nonce = nonce;
    secretKey.area = CryptoManager::Area::EL1;
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, "", secretKey);
    ASSERT_TRUE(secretKey.sKey.empty());
}

/**
* @tc.name: UpdateSecretMetaTest002
* @tc.desc: update meta with area EL1
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest002, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);
    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = TEST_BUNDLE_NAME;
    metaData.storeId = TEST_STORE_NAME;
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL1;
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey);
    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);
}

/**
* @tc.name: UpdateSecretMetaTest003
* @tc.desc: update meta with area EL2
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest003, TestSize.Level0)
{
    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = TEST_BUNDLE_NAME;
    metaData.storeId = TEST_STORE_NAME;
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL2;
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey);
    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);
}

/**
* @tc.name: UpdateSecretMetaTest004
* @tc.desc: update meta with area EL3
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest004, TestSize.Level0)
{
    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = TEST_BUNDLE_NAME;
    metaData.storeId = TEST_STORE_NAME;
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL3;
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey);
    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);
}

/**
* @tc.name: UpdateSecretMetaTest005
* @tc.desc: update meta with area EL4
* @tc.type: FUNC
*/
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest005, TestSize.Level0)
{
    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = TEST_BUNDLE_NAME;
    metaData.storeId = TEST_STORE_NAME;
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL4;
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey);
    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);
}

/**
 * @tc.name: UpdateSecretMetaTest006
 * @tc.desc: Test UpdateSecretMeta with MetaDataSaver parameter - batch save mode
 * @tc.type: FUNC
 */
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest006, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = TEST_BUNDLE_NAME;
    metaData.storeId = std::string(TEST_STORE_NAME) + "_batch";
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL1;

    // Test with MetaDataSaver for batch saving
    MetaDataSaver saver(false); // synchronous save for test
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey, saver);

    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);

    // Verify saver has the entry
    EXPECT_EQ(saver.Size(), 1u);
    EXPECT_TRUE(saver.Flush());
}

/**
 * @tc.name: UpdateSecretMetaTest007
 * @tc.desc: Test UpdateSecretMeta with MetaDataSaver and empty password
 * @tc.type: FUNC
 */
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest007, TestSize.Level0)
{
    std::vector<uint8_t> emptyPassword;
    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.area = CryptoManager::Area::EL1;

    MetaDataSaver saver(false);
    CryptoManager::GetInstance().UpdateSecretMeta(emptyPassword, metaData, "", secretKey, saver);

    // Should not add anything when password is empty
    EXPECT_TRUE(secretKey.sKey.empty());
    EXPECT_EQ(saver.Size(), 0u);
}

/**
 * @tc.name: UpdateSecretMetaTest008
 * @tc.desc: Test UpdateSecretMeta with MetaDataSaver when nonce exists and area >= 0
 * @tc.type: FUNC
 */
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest008, TestSize.Level0)
{
    SecretKeyMetaData secretKey;
    secretKey.nonce = nonce;
    secretKey.area = CryptoManager::Area::EL1;

    StoreMetaData metaData;
    metaData.area = CryptoManager::Area::EL2;

    MetaDataSaver saver(false);
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, "", secretKey, saver);

    // Should not update when nonce exists and area >= 0
    EXPECT_TRUE(secretKey.sKey.empty());
    EXPECT_EQ(saver.Size(), 0u);
}

/**
 * @tc.name: UpdateSecretMetaTest009
 * @tc.desc: Test UpdateSecretMeta backward compatibility (without saver) uses internal saver
 * @tc.type: FUNC
 */
HWTEST_F(CryptoManagerTest, UpdateSecretMetaTest009, TestSize.Level0)
{
    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    SecretKeyMetaData secretKey;
    StoreMetaData metaData;
    metaData.bundleName = std::string(TEST_BUNDLE_NAME) + "_compat";
    metaData.storeId = TEST_STORE_NAME;
    metaData.user = TEST_USERID;
    metaData.area = CryptoManager::Area::EL1;

    // Call version without MetaDataSaver - should use internal saver
    CryptoManager::GetInstance().UpdateSecretMeta(randomKey, metaData, metaData.GetSecretKey(), secretKey);

    ASSERT_FALSE(secretKey.sKey.empty());
    ASSERT_FALSE(secretKey.nonce.empty());
    ASSERT_EQ(secretKey.area, metaData.area);
}

} // namespace OHOS::Test
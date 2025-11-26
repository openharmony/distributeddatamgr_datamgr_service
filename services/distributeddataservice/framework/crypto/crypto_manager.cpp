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
#define LOG_TAG "CryptoManager"
#include "crypto/crypto_manager.h"

#include <cstring>
#include <string>

#include "hks_api.h"
#include "hks_param.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "securec.h"

namespace OHOS::DistributedData {
static constexpr int32_t NONCE_SIZE = 12;
static constexpr const char *ROOT_KEY_ALIAS = "distributed_db_root_key";
static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata";

using system_clock = std::chrono::system_clock;

CryptoManager::CryptoManager()
{
    vecRootKeyAlias_ = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    vecNonce_ = std::vector<uint8_t>(HKS_BLOB_TYPE_NONCE, HKS_BLOB_TYPE_NONCE + strlen(HKS_BLOB_TYPE_NONCE));
    vecAad_ = std::vector<uint8_t>(HKS_BLOB_TYPE_AAD, HKS_BLOB_TYPE_AAD + strlen(HKS_BLOB_TYPE_AAD));
}

CryptoManager::~CryptoManager()
{
}

CryptoManager &CryptoManager::GetInstance()
{
    static CryptoManager instance;
    return instance;
}

struct HksParam aes256Param[] = {
    { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
    { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
    { .tag = HKS_TAG_DIGEST, .uint32Param = HKS_DIGEST_NONE },
    { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
    { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
};

bool AddHksParams(HksParamSet *params, CryptoManager::ParamConfig &paramConfig)
{
    if (paramConfig.nonce.empty()) {
        uint8_t nonce[NONCE_SIZE] = {0};
        struct HksBlob blobNonce = { .size = NONCE_SIZE, .data = nonce };
        auto result = HksGenerateRandom(nullptr, &blobNonce);
        if (result != HKS_SUCCESS) {
            ZLOGE("HksGenerateRandom failed with error %{public}d", result);
            return false;
        }
        std::vector<uint8_t> nonceContent(blobNonce.data, blobNonce.data + blobNonce.size);
        paramConfig.nonce = nonceContent;
    }

    struct HksBlob blobAad = { uint32_t(paramConfig.aadValue.size()),
        const_cast<uint8_t *>(paramConfig.aadValue.data()) };
    std::vector<HksParam> hksParam = {
        { .tag = HKS_TAG_PURPOSE, .uint32Param = paramConfig.purpose },
        { .tag = HKS_TAG_NONCE,
            .blob = { uint32_t(paramConfig.nonce.size()), const_cast<uint8_t *>(paramConfig.nonce.data()) } },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = paramConfig.storageLevel },
    };

    if (paramConfig.storageLevel > HKS_AUTH_STORAGE_LEVEL_DE) {
        hksParam.emplace_back(
            HksParam { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = std::atoi(paramConfig.userId.c_str()) });
    }

    auto ret = HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksAddParams(params, hksParam.data(), hksParam.size());
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    return true;
}

int32_t GetRootKeyParams(HksParamSet *&params, uint32_t storageLevel, const std::string &userId)
{
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return ret;
    }

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
        hksParam.emplace_back(HksParam { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = std::atoi(userId.c_str()) });
    }

    ret = HksAddParams(params, hksParam.data(), hksParam.size());
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return ret;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&params);
    }
    return ret;
}

int32_t CryptoManager::GenerateRootKey()
{
    return GenerateRootKey(HKS_AUTH_STORAGE_LEVEL_DE, DEFAULT_USER) == HKS_SUCCESS ? ErrCode::SUCCESS : ErrCode::ERROR;
}

int32_t CryptoManager::GenerateRootKey(uint32_t storageLevel, const std::string &userId)
{
    ZLOGI("GenerateRootKey, storageLevel=%{public}u, userId=%{public}s", storageLevel, userId.c_str());
    struct HksParamSet *params = nullptr;
    int32_t ret = GetRootKeyParams(params, storageLevel, userId);
    if (ret != HKS_SUCCESS || params == nullptr) {
        ZLOGE("GetRootKeyParams failed with error %{public}d, storageLevel:%{public}u, userId:%{public}s", ret,
            storageLevel, userId.c_str());
        return ErrCode::ERROR;
    }
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    ret = HksGenerateKey(&rootKeyName, params, nullptr);
    HksFreeParamSet(&params);
    if (ret == HKS_SUCCESS) {
        ZLOGI("GenerateRootKey Succeed. storageLevel:%{public}u, userId:%{public}s", storageLevel, userId.c_str());
        return ErrCode::SUCCESS;
    }
    ZLOGE("HksGenerateKey failed with error %{public}d, storageLevel:%{public}u, userId:%{public}s",
        ret, storageLevel, userId.c_str());
    return ErrCode::ERROR;
}

int32_t CryptoManager::CheckRootKey()
{
    ZLOGI("CheckRootKey.");
    return CheckRootKey(HKS_AUTH_STORAGE_LEVEL_DE, DEFAULT_USER);
}

int32_t CryptoManager::CheckRootKey(uint32_t storageLevel, const std::string &userId)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = GetRootKeyParams(params, storageLevel, userId);
    if (ret != HKS_SUCCESS) {
        ZLOGE("GetRootKeyParams failed with error %{public}d, storageLevel: %{public}u", ret, storageLevel);
        return ErrCode::ERROR;
    }

    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    ret = HksKeyExist(&rootKeyName, params);
    HksFreeParamSet(&params);
    if (ret == HKS_SUCCESS) {
        return ErrCode::SUCCESS;
    }
    ZLOGE("HksKeyExist failed with error %{public}d, storageLevel: %{public}u", ret, storageLevel);
    if (ret == HKS_ERROR_NOT_EXIST) {
        return ErrCode::NOT_EXIST;
    }
    return ErrCode::ERROR;
}

uint32_t CryptoManager::GetStorageLevel(int32_t area)
{
    if (area >= EL4 && area <= EL5) {
        return HKS_AUTH_STORAGE_LEVEL_ECE;
    }
    if (area >= EL2 && area <= EL3) {
        return HKS_AUTH_STORAGE_LEVEL_CE;
    }
    return HKS_AUTH_STORAGE_LEVEL_DE;
}

int32_t CryptoManager::PrepareRootKey(uint32_t storageLevel, const std::string &userId)
{
    if (storageLevel == HKS_AUTH_STORAGE_LEVEL_DE) {
        return ErrCode::SUCCESS;
    }
    auto status = CheckRootKey(storageLevel, userId);
    if (status == ErrCode::SUCCESS) {
        return ErrCode::SUCCESS;
    }
    if (status == ErrCode::NOT_EXIST && GenerateRootKey(storageLevel, userId) == ErrCode::SUCCESS) {
        ZLOGI("GenerateRootKey success.");
        return ErrCode::SUCCESS;
    }
    ZLOGW("GenerateRootKey failed, storageLevel:%{public}u, userId:%{public}s, status:%{public}d", storageLevel,
        userId.c_str(), status);
    return status;
}

std::vector<uint8_t> CryptoManager::Encrypt(const std::vector<uint8_t> &password, CryptoParams &encryptParams)
{
    encryptParams.area = encryptParams.area < 0 ? Area::EL1 : encryptParams.area;
    uint32_t storageLevel = GetStorageLevel(encryptParams.area);
    if (PrepareRootKey(storageLevel, encryptParams.userId) != ErrCode::SUCCESS) {
        return {};
    }

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed with error %{public}d", ret);
        return {};
    }

    ParamConfig paramConfig = {
        .purpose = HKS_KEY_PURPOSE_ENCRYPT,
        .storageLevel = storageLevel,
        .userId = encryptParams.userId,
        .nonce = encryptParams.nonce,
        .aadValue = vecAad_
    };
    if (!AddHksParams(params, paramConfig)) {
        return {};
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }

    if (encryptParams.keyAlias.empty()) {
        encryptParams.keyAlias = vecRootKeyAlias_;
    }
    struct HksBlob keyName = { uint32_t(encryptParams.keyAlias.size()),
        const_cast<uint8_t *>(encryptParams.keyAlias.data())};
    struct HksBlob plainKey = { uint32_t(password.size()), const_cast<uint8_t *>(password.data()) };
    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    ret = HksEncrypt(&keyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed with error %{public}d", ret);
        return {};
    }

    if (encryptParams.nonce.empty()) {
        encryptParams.nonce = paramConfig.nonce;
    }
    std::vector<uint8_t> encryptedKey(cipherText.data, cipherText.data + cipherText.size);
    std::fill(cipherBuf, cipherBuf + sizeof(cipherBuf), 0);
    return encryptedKey;
}

std::vector<uint8_t> CryptoManager::Decrypt(const std::vector<uint8_t> &source, CryptoParams &decryptParams)
{
    uint32_t storageLevel = GetStorageLevel(decryptParams.area);
    if (PrepareRootKey(storageLevel, decryptParams.userId) != ErrCode::SUCCESS) {
        return {};
    }

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed with error %{public}d", ret);
        return {};
    }

    ParamConfig paramConfig = {
        .purpose = HKS_KEY_PURPOSE_DECRYPT,
        .storageLevel = storageLevel,
        .userId = decryptParams.userId,
        .nonce = decryptParams.nonce.empty() ? vecNonce_ : decryptParams.nonce,
        .aadValue = vecAad_
    };
    if (!AddHksParams(params, paramConfig)) {
        return {};
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }

    if (decryptParams.keyAlias.empty()) {
        decryptParams.keyAlias = vecRootKeyAlias_;
    }
    struct HksBlob keyName = { uint32_t(decryptParams.keyAlias.size()),
        const_cast<uint8_t *>(decryptParams.keyAlias.data()) };
    struct HksBlob encryptedKeyBlob = { uint32_t(source.size()), const_cast<uint8_t*>(source.data()) };
    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };

    ret = HksDecrypt(&keyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksDecrypt failed with error %{public}d", ret);
        return {};
    }
    std::vector<uint8_t> password(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    return password;
}

void CryptoManager::UpdateSecretMeta(const std::vector<uint8_t> &password, const StoreMetaData &metaData,
    const std::string &metaKey, SecretKeyMetaData &secretKey)
{
    if (password.empty() || (!secretKey.nonce.empty() && secretKey.area >= 0)) {
        return;
    }
    CryptoParams encryptParams = { .area = metaData.area, .userId = metaData.user,
        .nonce = secretKey.nonce };
    auto encryptKey = Encrypt(password, encryptParams);
    if (encryptKey.empty()) {
        return;
    }
    secretKey.sKey = encryptKey;
    secretKey.nonce = encryptParams.nonce;
    secretKey.area = metaData.area;
    auto time = system_clock::to_time_t(system_clock::now());
    secretKey.time = { reinterpret_cast<uint8_t *>(&time),
        reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    MetaDataManager::GetInstance().SaveMeta(metaKey, secretKey, true);
}

bool BuildImportKeyParams(struct HksParamSet *&params)
{
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet failed with error %{public}d", ret);
        return false;
    }
    struct HksParam purposeParam[] = {
        {.tag = HKS_TAG_IS_KEY_ALIAS, .boolParam = true},
        {.tag = HKS_TAG_KEY_GENERATE_TYPE, .uint32Param = HKS_KEY_GENERATE_TYPE_DEFAULT},
        {.tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT},
        {.tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE},
    };
    ret = HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksAddParams(params, purposeParam, sizeof(purposeParam) / sizeof(purposeParam[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    return true;
}

bool CryptoManager::ImportKey(const std::vector<uint8_t> &key, const std::vector<uint8_t> &keyAlias)
{
    std::lock_guard<std::mutex> lock(mutex_);
    struct HksBlob hksKey = { key.size(), const_cast<uint8_t *>(key.data()) };
    struct HksParamSet *params = nullptr;
    if (!BuildImportKeyParams(params)) {
        return false;
    }

    struct HksBlob keyName = { uint32_t(keyAlias.size()), const_cast<uint8_t *>(keyAlias.data()) };
    int32_t ret = HksImportKey(&keyName, params, &hksKey);
    if (ret != HKS_SUCCESS) {
        ZLOGE("Import key failed: %{public}d.", ret);
        HksFreeParamSet(&params);
        return false;
    }
    HksFreeParamSet(&params);
    return true;
}

bool CryptoManager::DeleteKey(const std::vector<uint8_t> &keyAlias)
{
    struct HksBlob keyName = { uint32_t(keyAlias.size()), const_cast<uint8_t *>(keyAlias.data()) };
    struct HksParamSet *params = nullptr;
    if (!BuildImportKeyParams(params)) {
        return false;
    }
    int32_t ret = HksDeleteKey(&keyName, params);
    if (ret != HKS_SUCCESS) {
        HksFreeParamSet(&params);
        return false;
    }
    return true;
}

std::vector<uint8_t> CryptoManager::Random(uint32_t length)
{
    std::vector<uint8_t> value(length, 0);
    struct HksBlob blobValue = { .size = length, .data = value.data() };
    auto ret = HksGenerateRandom(nullptr, &blobValue);
    if (ret != HKS_SUCCESS || value.empty()) {
        ZLOGE("HksGenerateRandom failed, status: %{public}d", ret);
        return {};
    }
    return value;
}
} // namespace OHOS::DistributedData
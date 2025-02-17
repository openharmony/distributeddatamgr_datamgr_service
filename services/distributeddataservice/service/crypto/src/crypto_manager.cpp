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
#include "crypto_manager.h"

#include <cstring>
#include <string>

#include "hks_api.h"
#include "hks_param.h"
#include "log_print.h"
#include "securec.h"
namespace OHOS::DistributedData {
std::vector<uint8_t> backupNonce_{};
std::vector<uint8_t> vecAad_{};
std::vector<uint8_t> vecCloneKeyAlias_{};
std::vector<uint8_t> vecNonce_{};
std::vector<uint8_t> vecRootKeyAlias_{};

CryptoManager::CryptoManager()
{
    vecRootKeyAlias_ = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    vecCloneKeyAlias_ = std::vector<uint8_t>(BACKUP_KEY_ALIAS, BACKUP_KEY_ALIAS + strlen(BACKUP_KEY_ALIAS));
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

bool AddHksParams(struct HksParamSet *params, CryptoManager::ParamConfig paramConfig)
{
    struct HksBlob blobNonce;
    struct HksBlob keyName;
    if (paramConfig.keyType == RootKeys::ROOT_KEY) {
        blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    } else if (paramConfig.keyType == RootKeys::CLONE_KEY) {
        blobNonce = { uint32_t(backupNonce_.size()), backupNonce_.data() };
        keyName = { uint32_t(vecCloneKeyAlias_.size()), vecCloneKeyAlias_.data() };
    } else {
        return false;
    }
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    std::vector<HksParam> hksParam = {
        { .tag = HKS_TAG_PURPOSE, .uint32Param = paramConfig.purpose },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = paramConfig.storageLevel },
    };
    if (paramConfig.storageLevel > HKS_AUTH_STORAGE_LEVEL_DE) {
        hksParam.emplace_back(HksParam { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = std::stoi(paramConfig.userId) });
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
        hksParam.emplace_back(HksParam { .tag = HKS_TAG_SPECIFIC_USER_ID, .int32Param = std::stoi(userId) });
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
    if (ret != HKS_SUCCESS) {
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

std::vector<uint8_t> CryptoManager::Encrypt(const std::vector<uint8_t> &key, int32_t area, const std::string &userId)
{
    return EncryptInner(key, RootKeys::ROOT_KEY, area, userId);
}

std::vector<uint8_t> CryptoManager::EncryptCloneKey(const std::vector<uint8_t> &key, int32_t area, const std::string &userId)
{
    return EncryptInner(key, RootKeys::CLONE_KEY, area, userId);
}

std::vector<uint8_t> CryptoManager::EncryptInner(const std::vector<uint8_t> &key, const RootKeys type,
   int32_t area, const std::string &userId)
{
    uint32_t storageLevel = GetStorageLevel(area);
    if (PrepareRootKey(storageLevel, userId) != ErrCode::SUCCESS) {
        return {};
    }

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return {};
    }
    ParamConfig paramConfig = {
        .keyType = type,
        .purpose = HKS_KEY_PURPOSE_ENCRYPT,
        .storageLevel = storageLevel,
        .userId = userId
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

    uint8_t cipherBuf[256] = { 0 };
    struct HksBlob cipherText = { sizeof(cipherBuf), cipherBuf };
    struct HksBlob keyName;
    if (type == RootKeys::ROOT_KEY) {
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    } else if (type == RootKeys::CLONE_KEY) {
        keyName = { uint32_t(vecCloneKeyAlias_.size()), vecCloneKeyAlias_.data() };
    }
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    ret = HksEncrypt(&keyName, params, &plainKey, &cipherText);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed with error %{public}d", ret);
        return {};
    }

    std::vector<uint8_t> encryptedKey(cipherText.data, cipherText.data + cipherText.size);
    (void)memset_s(cipherBuf, sizeof(cipherBuf), 0, sizeof(cipherBuf));
    return encryptedKey;
}

bool CryptoManager::Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area, const std::string &userId)
{
    return DecryptInner(source, key, RootKeys::ROOT_KEY, area, userId);
}

bool CryptoManager::DecryptCloneKey(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area, const std::string &userId)
{
    return DecryptInner(source, key, RootKeys::CLONE_KEY, area, userId);
}

bool CryptoManager::DecryptInner(std::vector<uint8_t> &source, std::vector<uint8_t> &key, RootKeys type, int32_t area, const std::string &userId)
{
    uint32_t storageLevel = GetStorageLevel(area);
    if (PrepareRootKey(storageLevel, userId) != ErrCode::SUCCESS) {
        return false;
    }
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return false;
    }

    ParamConfig paramConfig = {
        .keyType = type,
        .purpose = HKS_KEY_PURPOSE_DECRYPT,
        .storageLevel = storageLevel,
        .userId = userId
    };
    if (!AddHksParams(params, paramConfig)) {
        return false;
    }

    ret = HksBuildParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }

    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
    struct HksBlob keyName;
    if (type == RootKeys::ROOT_KEY) {
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    } else if (type == RootKeys::CLONE_KEY) {
        keyName = { uint32_t(vecCloneKeyAlias_.size()), vecCloneKeyAlias_.data() };
    }
    struct HksBlob encryptedKeyBlob = { uint32_t(source.size()), source.data() };
    ret = HksDecrypt(&keyName, params, &encryptedKeyBlob, &plainKeyBlob);
    (void)HksFreeParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksDecrypt failed with error %{public}d", ret);
        return false;
    }

    key.assign(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    (void)memset_s(plainBuf, sizeof(plainBuf), 0, sizeof(plainBuf));
    return true;
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

bool CryptoManager::ImportCloneKey(std::vector<uint8_t> &key, std::vector<uint8_t> &iv)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ZLOGI("ImportCloneKey enter.");
    backupNonce_ = std::vector<uint8_t>(iv.begin(), iv.end());
    struct HksBlob hksKey = { key.size(), key.data()};
    struct HksParamSet *params = nullptr;
    if (!BuildImportKeyParams(params)) {
        ZLOGE("Build import key params failed.");
        return false;
    }

    struct HksBlob backupKeyName = { uint32_t(vecCloneKeyAlias_.size()), vecCloneKeyAlias_.data() };
    int32_t ret = HksImportKey(&backupKeyName, params, &hksKey);
    if (ret != HKS_SUCCESS) {
        ZLOGE("Import key failed: %{public}d.", ret);
        HksFreeParamSet(&params);
        return false;
    }
    HksFreeParamSet(&params);
    return true;
}
} // namespace OHOS::DistributedData
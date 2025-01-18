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
#include <sstream>
#define LOG_TAG "CryptoManager"
#include "crypto_manager.h"

#include <cstring>

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
    { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
};

bool AddParams(struct HksParamSet *params, RootKeys type)
{
    struct HksBlob blobNonce;
    struct HksBlob keyName;
    if (type == RootKeys::ROOT_KEY) {
        blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    } else if (type == RootKeys::BACKUP_KEY) {
        blobNonce = { uint32_t(backupNonce_.size()), backupNonce_.data() };
        keyName = { uint32_t(vecCloneKeyAlias_.size()), vecCloneKeyAlias_.data() };
    } else {
        return false;
    }
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
    };

    auto ret = HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    return true;
}

int32_t GetRootKeyParams(HksParamSet *&params)
{
    ZLOGI("GetRootKeyParams.");
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return ret;
    }

    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_AUTH_STORAGE_LEVEL, .uint32Param = HKS_AUTH_STORAGE_LEVEL_DE },
    };

    ret = HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0]));
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
    ZLOGI("GenerateRootKey.");
    struct HksParamSet *params = nullptr;
    int32_t ret = GetRootKeyParams(params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("GetRootKeyParams failed with error %{public}d", ret);
        return ErrCode::ERROR;
    }
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    ret = HksGenerateKey(&rootKeyName, params, nullptr);
    HksFreeParamSet(&params);
    if (ret == HKS_SUCCESS) {
        ZLOGI("GenerateRootKey Succeed.");
        return ErrCode::SUCCESS;
    }

    ZLOGE("HksGenerateKey failed with error %{public}d", ret);
    return ErrCode::ERROR;
}

int32_t CryptoManager::CheckRootKey()
{
    ZLOGI("CheckRootKey.");
    struct HksParamSet *params = nullptr;
    int32_t ret = GetRootKeyParams(params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("GetRootKeyParams failed with error %{public}d", ret);
        return ErrCode::ERROR;
    }

    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    ret = HksKeyExist(&rootKeyName, params);
    HksFreeParamSet(&params);
    if (ret == HKS_SUCCESS) {
        return ErrCode::SUCCESS;
    }
    ZLOGE("HksKeyExist failed with error %{public}d", ret);
    if (ret == HKS_ERROR_NOT_EXIST) {
        return ErrCode::NOT_EXIST;
    }
    return ErrCode::ERROR;
}

std::vector<uint8_t> CryptoManager::Encrypt(const std::vector<uint8_t> &key)
{
    return EncryptInner(key, RootKeys::ROOT_KEY);
}

std::vector<uint8_t> CryptoManager::EncryptCloneKey(const std::vector<uint8_t> &key)
{
    return EncryptInner(key, RootKeys::BACKUP_KEY);
}

std::vector<uint8_t> CryptoManager::EncryptInner(const std::vector<uint8_t> &key, const RootKeys type)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return {};
    }
    if (!AddParams(params, type)) {
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
    } else if (type == RootKeys::BACKUP_KEY) {
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

bool CryptoManager::Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key)
{
    return DecryptInner(source, key, RootKeys::ROOT_KEY);
}

bool CryptoManager::DecryptCloneKey(std::vector<uint8_t> &source, std::vector<uint8_t> &key)
{
    return DecryptInner(source, key, RootKeys::BACKUP_KEY);
}

bool CryptoManager::DecryptInner(std::vector<uint8_t> &source, std::vector<uint8_t> &key, RootKeys type)
{
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return false;
    }

    if (!AddParams(params, type)) {
        return {};
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
    } else if (type == RootKeys::BACKUP_KEY) {
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

bool BuildImportKeyParams(struct HksParamSet *&params, HksBlob blobNonce)
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
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
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
    HksBlob nonce_ = { iv.size(), iv.data() };
    struct HksBlob hksKey = { key.size(), key.data()};
    struct HksParamSet *params = nullptr;
    if (!BuildImportKeyParams(params, nonce_)) {
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
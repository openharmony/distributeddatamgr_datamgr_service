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
CryptoManager::CryptoManager()
{
    vecRootKeyAlias_ = std::vector<uint8_t>(ROOT_KEY_ALIAS, ROOT_KEY_ALIAS + strlen(ROOT_KEY_ALIAS));
    vecBackupKeyAlias_ = std::vector<uint8_t>(BACKUP_KEY_ALIAS, BACKUP_KEY_ALIAS + strlen(BACKUP_KEY_ALIAS));
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

std::vector<uint8_t> CryptoManager::BackupKeyEncrypt(const std::vector<uint8_t> &key)
{
    return EncryptInner(key, RootKeys::BACKUP_KEY);
}

std::vector<uint8_t> CryptoManager::EncryptInner(const std::vector<uint8_t> &key, const RootKeys type)
{
    struct HksBlob blobNonce;
    struct HksBlob keyName;
    if (type == RootKeys::ROOT_KEY) {
        blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    }
    if (type == RootKeys::BACKUP_KEY) {
        blobNonce = { uint32_t(backupNonce_.size()), backupNonce_.data() };
        keyName = { uint32_t(vecBackupKeyAlias_.size()), vecBackupKeyAlias_.data() };
    }
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return {};
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
    };

    if (HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0])) != HKS_SUCCESS ||
        HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0])) != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&params);
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

bool CryptoManager::BackupKeyDecrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key)
{
    return DecryptInner(source, key, RootKeys::BACKUP_KEY);
}

bool CryptoManager::DecryptInner(std::vector<uint8_t> &source, std::vector<uint8_t> &key, const RootKeys type)
{
    struct HksBlob blobNonce;
    struct HksBlob keyName;
    if (type == RootKeys::ROOT_KEY) {
        blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
        keyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    } else if (type == RootKeys::BACKUP_KEY) {
        blobNonce = { uint32_t(backupNonce_.size()), backupNonce_.data() };
        keyName = { uint32_t(vecBackupKeyAlias_.size()), vecBackupKeyAlias_.data() };
    }
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), &(vecAad_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(source.size()), source.data() };

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return false;
    }
    struct HksParam hksParam[] = {
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
    };
    if (HksAddParams(params, hksParam, sizeof(hksParam) / sizeof(hksParam[0])) != HKS_SUCCESS ||
        HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0])) != HKS_SUCCESS) {
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

    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
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

void ConvertDecStrToVec(const std::string &inData, std::vector<char>&outData, int size)
{
    std::stringstream ss(inData);
    std::string token;
    while (size-- > 0 && getline(ss, token, ',')) {
        char num = std::stoi(token);
        outData.push_back(num);
    }
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
    };
    HksAddParams(params, aes256Param, sizeof(aes256Param) / sizeof(aes256Param[0]));
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

bool CryptoManager::ImportBackupKey(const std::string &key, const std::string &ivStr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ZLOGI("ImportBackupKey enter.");
    uint8_t aesKey[KEY_SIZE] = { 0 };
    std::vector<char> result;

    ConvertDecStrToVec(key, result, KEY_SIZE);
    if (result.size() != KEY_SIZE) {
        ZLOGE("ImportKey failed, key length not correct.");
        (void)memset_s(aesKey, sizeof(aesKey), 0, sizeof(aesKey));
        return false;
    }
    std::copy(result.begin(), result.begin() + KEY_SIZE, aesKey);
    result.clear();

    ConvertDecStrToVec(ivStr, result, AES_256_NONCE_SIZE);
    if (result.size() != AES_256_NONCE_SIZE) {
        ZLOGE("ImportKey failed, iv length not correct.");
        (void)memset_s(aesKey, sizeof(aesKey), 0, sizeof(aesKey));
        return false;
    }
    backupNonce_ = std::vector<uint8_t>(result.begin(), result.end());
    result.clear();
    struct HksBlob hksKey = { KEY_SIZE, aesKey};
    struct HksParamSet *params = nullptr;

    if (BuildImportKeyParams(params) == false) {
        (void)memset_s(aesKey, sizeof(aesKey), 0, sizeof(aesKey));
        return false;
    }

    struct HksBlob backupKeyName = { uint32_t(vecBackupKeyAlias_.size()), vecBackupKeyAlias_.data() };

    int32_t ret = HksImportKey(&backupKeyName, params, &hksKey);
    (void)memset_s(aesKey, sizeof(aesKey), 0, sizeof(aesKey));
    if (ret != HKS_SUCCESS) {
        ZLOGE("ImportKey failed: %{public}d.", ret);
    }
    HksFreeParamSet(&params);
    return true;
}
} // namespace OHOS::DistributedData
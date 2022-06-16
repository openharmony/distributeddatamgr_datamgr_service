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

#include "hks_api.h"
#include "hks_param.h"
#include "log_print.h"
#include "securec.h"
namespace OHOS::DistributedData {
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

int32_t CryptoManager::GenerateRootKey()
{
    ZLOGI("GenerateRootKey.");
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksParamSet *param = nullptr;
    int32_t ret = HksInitParamSet(&param);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return ret;
    }

    struct HksParam genKeyParams[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_KEY_SIZE, .uint32Param = HKS_AES_KEY_SIZE_256 },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT | HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
    };

    ret = HksAddParams(param, genKeyParams, sizeof(genKeyParams) / sizeof(genKeyParams[0]));
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksAddParams failed with error %{public}d", ret);
        HksFreeParamSet(&param);
        return ret;
    }

    ret = HksBuildParamSet(&param);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksBuildParamSet failed with error %{public}d", ret);
        HksFreeParamSet(&param);
        return ret;
    }

    ret = HksGenerateKey(&rootKeyName, param, nullptr);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksGenerateKey failed with error %{public}d", ret);
        HksFreeParamSet(&param);
        return ret;
    }
    HksFreeParamSet(&param);
    ZLOGI("GenerateRootKey Succeed.");
    return HKS_SUCCESS;
}

bool CryptoManager::IsExistRootKey()
{
    return false;
}

std::vector<uint8_t> CryptoManager::Encrypt(const std::vector<uint8_t> &key)
{
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), vecAad_.data() };
    struct HksBlob blobNonce = { uint32_t(vecNonce_.size()), vecNonce_.data() };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), vecRootKeyAlias_.data() };
    struct HksBlob plainKey = { uint32_t(key.size()), const_cast<uint8_t *>(key.data()) };
    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return {};
    }
    struct HksParam param[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_ENCRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
    };
    ret = HksAddParams(params, param, sizeof(param) / sizeof(param[0]));
    if (ret != HKS_SUCCESS) {
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
    ret = HksEncrypt(&rootKeyName, params, &plainKey, &cipherText);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksEncrypt failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return {};
    }
    (void)HksFreeParamSet(&params);
    std::vector<uint8_t> encryptedKey(cipherText.data, cipherText.data + cipherText.size);
    (void)memset_s(cipherBuf, sizeof(cipherBuf), 0, sizeof(cipherBuf));
    return encryptedKey;
}

bool CryptoManager::Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key)
{
    struct HksBlob blobAad = { uint32_t(vecAad_.size()), &(vecAad_[0]) };
    struct HksBlob blobNonce = { uint32_t(vecNonce_.size()), &(vecNonce_[0]) };
    struct HksBlob rootKeyName = { uint32_t(vecRootKeyAlias_.size()), &(vecRootKeyAlias_[0]) };
    struct HksBlob encryptedKeyBlob = { uint32_t(source.size()), source.data() };

    struct HksParamSet *params = nullptr;
    int32_t ret = HksInitParamSet(&params);
    if (ret != HKS_SUCCESS) {
        ZLOGE("HksInitParamSet() failed with error %{public}d", ret);
        return false;
    }
    struct HksParam param[] = {
        { .tag = HKS_TAG_ALGORITHM, .uint32Param = HKS_ALG_AES },
        { .tag = HKS_TAG_PURPOSE, .uint32Param = HKS_KEY_PURPOSE_DECRYPT },
        { .tag = HKS_TAG_DIGEST, .uint32Param = 0 },
        { .tag = HKS_TAG_BLOCK_MODE, .uint32Param = HKS_MODE_GCM },
        { .tag = HKS_TAG_PADDING, .uint32Param = HKS_PADDING_NONE },
        { .tag = HKS_TAG_NONCE, .blob = blobNonce },
        { .tag = HKS_TAG_ASSOCIATED_DATA, .blob = blobAad },
    };
    ret = HksAddParams(params, param, sizeof(param) / sizeof(param[0]));
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
    uint8_t plainBuf[256] = { 0 };
    struct HksBlob plainKeyBlob = { sizeof(plainBuf), plainBuf };
    ret = HksDecrypt(&rootKeyName, params, &encryptedKeyBlob, &plainKeyBlob);
    if (ret != HKS_SUCCESS) {
        ZLOGW("HksDecrypt failed with error %{public}d", ret);
        HksFreeParamSet(&params);
        return false;
    }
    (void)HksFreeParamSet(&params);

    key.assign(plainKeyBlob.data, plainKeyBlob.data + plainKeyBlob.size);
    (void)memset_s(plainBuf, sizeof(plainBuf), 0, sizeof(plainBuf));
    return true;
}
} // namespace OHOS::DistributedData
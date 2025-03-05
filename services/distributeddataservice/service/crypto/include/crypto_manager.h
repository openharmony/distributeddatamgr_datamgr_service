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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_CRYPTO_CRYPTO_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_CRYPTO_CRYPTO_MANAGER_H

#include <cstdint>
#include <mutex>
#include <vector>
#include "hks_param.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "visibility.h"

namespace OHOS::DistributedData {
static constexpr int32_t DEFAULT_ENCRYPTION_LEVEL = 1;
static constexpr const char *DEFAULT_USER = "0";
class API_EXPORT CryptoManager {
public:
    enum SecretKeyType {
        LOCAL_SECRET_KEY,
        CLONE_SECRET_KEY,
    };
    struct ParamConfig {
        const std::vector<uint8_t> &nonce;
        uint32_t purpose;
        uint32_t storageLevel;
        std::string userId;
    };
    struct EncryptParams {
        const std::vector<uint8_t> keyAlias;
        const std::vector<uint8_t> nonce;
    };
    enum Area : int32_t {
        EL0,
        EL1,
        EL2,
        EL3,
        EL4,
        EL5
    };
    static CryptoManager &GetInstance();
    int32_t GenerateRootKey();
    int32_t CheckRootKey();
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key);
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key, const EncryptParams &encryptParams);
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key, int32_t area, const std::string &userId);
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key,
        int32_t area, const std::string &userId, const EncryptParams &encryptParams
    );
    bool Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key, const EncryptParams &encryptParams);
    bool Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area, const std::string &userId);
    bool Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key,
        int32_t area, const std::string &userId, const EncryptParams &encryptParams
    );
    bool ImportKey(const std::vector<uint8_t> &key, const std::vector<uint8_t> &keyAlias);
    bool DeleteKey(const std::vector<uint8_t> &keyAlias);
    bool UpdateSecretKey(const StoreMetaData &meta, const std::vector<uint8_t> &password,
        SecretKeyType secretKeyType = LOCAL_SECRET_KEY);
    bool Decrypt(const StoreMetaData &meta, SecretKeyMetaData &secretKeyMeta, std::vector<uint8_t> &key,
        SecretKeyType secretKeyType = LOCAL_SECRET_KEY);

    enum ErrCode : int32_t {
        SUCCESS,
        NOT_EXIST,
        ERROR,
    };
private:
    static constexpr const char *ROOT_KEY_ALIAS = "distributed_db_root_key";
    static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
    static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata";
    static constexpr int KEY_SIZE = 32;
    static constexpr int AES_256_NONCE_SIZE = 32;
    static constexpr int HOURS_PER_YEAR = (24 * 365);

    int32_t GenerateRootKey(uint32_t storageLevel, const std::string &userId);
    int32_t CheckRootKey(uint32_t storageLevel, const std::string &userId);
    uint32_t GetStorageLevel(int32_t area);
    int32_t PrepareRootKey(uint32_t storageLevel, const std::string &userId);
    std::vector<uint8_t> EncryptInner(const std::vector<uint8_t> &key, const SecretKeyType type, int32_t area,
        const std::string &userId);
    bool DecryptInner(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area,
        const std::string &userId, std::vector<uint8_t> &keyAlias, std::vector<uint8_t> &nonce);
    bool AddHksParams(HksParamSet *params, CryptoManager::ParamConfig paramConfig);
    int32_t GetRootKeyParams(HksParamSet *&params, uint32_t storageLevel, const std::string &userId);
    bool BuildImportKeyParams(HksParamSet *&params);
    CryptoManager();
    std::vector<uint8_t> vecRootKeyAlias_{};
    std::vector<uint8_t> vecNonce_{};
    std::vector<uint8_t> vecAad_{};
    ~CryptoManager();
    std::mutex mutex_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_CRYPTO_CRYPTO_MANAGER_H

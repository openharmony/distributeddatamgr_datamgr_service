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

#include <mutex>
#include <cstdint>
#include <vector>
#include "hks_param.h"
#include "visibility.h"

namespace OHOS::DistributedData {
enum RootKeys {
    ROOT_KEY,
    CLONE_KEY,
};
static constexpr int32_t DEFAULT_ENCRYPTION_LEVEL = 1;
static constexpr const char *DEFAULT_USER = "0";
class API_EXPORT CryptoManager {
public:
    static CryptoManager &GetInstance();
    int32_t GenerateRootKey();
    int32_t CheckRootKey();
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key, int32_t area, const std::string &userId);
    std::vector<uint8_t> EncryptCloneKey(const std::vector<uint8_t> &key, int32_t area, const std::string &userId);
    bool Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area, const std::string &userId);
    bool DecryptCloneKey(std::vector<uint8_t> &source, std::vector<uint8_t> &key, int32_t area,
        const std::string &userId);
    bool ImportCloneKey(std::vector<uint8_t> &key, std::vector<uint8_t> &iv);
    struct ParamConfig {
        RootKeys keyType;
        uint32_t purpose;
        uint32_t storageLevel;
        const std::string &userId;
    };
    enum Area : int32_t {
        EL0,
        EL1,
        EL2,
        EL3,
        EL4,
        EL5
    };
    enum ErrCode : int32_t {
        SUCCESS,
        NOT_EXIST,
        ERROR,
    };
private:
    static constexpr const char *ROOT_KEY_ALIAS = "distributed_db_root_key";
    static constexpr const char *BACKUP_KEY_ALIAS = "distributed_db_backup_key";
    static constexpr const char *HKS_BLOB_TYPE_NONCE = "Z5s0Bo571KoqwIi6";
    static constexpr const char *HKS_BLOB_TYPE_AAD = "distributeddata";
    static constexpr int KEY_SIZE = 32;
    static constexpr int AES_256_NONCE_SIZE = 32;
    static constexpr int HOURS_PER_YEAR = (24 * 365);

    int32_t GenerateRootKey(uint32_t storageLevel, const std::string &userId);
    int32_t CheckRootKey(uint32_t storageLevel, const std::string &userId);
    uint32_t GetStorageLevel(int32_t area);
    int32_t PrepareRootKey(uint32_t storageLevel, const std::string &userId);
    std::vector<uint8_t> EncryptInner(const std::vector<uint8_t> &key, const RootKeys type, int32_t area,
        const std::string &userId);
    bool DecryptInner(std::vector<uint8_t> &source, std::vector<uint8_t> &key, const RootKeys type, int32_t area,
        const std::string &userId);
    bool AddHksParams(HksParamSet *params, CryptoManager::ParamConfig paramConfig);
    int32_t GetRootKeyParams(HksParamSet *&params, uint32_t storageLevel, const std::string &userId);
    bool BuildImportKeyParams(HksParamSet *&params);
    CryptoManager();
    ~CryptoManager();
    std::mutex mutex_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_CRYPTO_CRYPTO_MANAGER_H

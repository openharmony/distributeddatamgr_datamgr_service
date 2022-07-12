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
#include <vector>
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CryptoManager {
public:
    static CryptoManager &GetInstance();
    int32_t GenerateRootKey();
    int32_t CheckRootKey();
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &key);
    bool Decrypt(std::vector<uint8_t> &source, std::vector<uint8_t> &key);

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
    static constexpr int HOURS_PER_YEAR = (24 * 365);
    CryptoManager();
    ~CryptoManager();
    std::vector<uint8_t> vecRootKeyAlias_{};
    std::vector<uint8_t> vecNonce_{};
    std::vector<uint8_t> vecAad_{};
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SERVICE_CRYPTO_CRYPTO_MANAGER_H

/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "CryptoUpgrade"
#include "crypto_upgrade.h"

#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"

namespace OHOS::DistributedData {
using system_clock = std::chrono::system_clock;

CryptoUpgrade &CryptoUpgrade::GetInstance()
{
    static CryptoUpgrade instance;
    return instance;
}

bool CryptoUpgrade::UpdatePassword(const StoreMetaData &meta, const std::vector<uint8_t> &password,
    SecretKeyType secretKeyType)
{
    if (!meta.isEncrypt) {
        return false;
    }
    SecretKeyMetaData secretKey;
    secretKey.storeType = meta.storeType;
    secretKey.area = meta.area;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(password, meta.area, meta.user);
    auto time = system_clock::to_time_t(system_clock::now());
    secretKey.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    if (secretKeyType == LOCAL_SECRET_KEY) {
        return MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), secretKey, true);
    } else {
        return MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), secretKey, true);
    }
}

bool CryptoUpgrade::Decrypt(const StoreMetaData &meta, SecretKeyMetaData &secretKeyMeta, std::vector<uint8_t> &key,
    SecretKeyType secretKeyType)
{
    if (secretKeyMeta.area < 0) {
        ZLOGI("Decrypt old secret key");
        if (CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, key, DEFAULT_ENCRYPTION_LEVEL, DEFAULT_USER)) {
            StoreMetaData metaData;
            if (MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), metaData, true)) {
                ZLOGI("Upgrade secret key");
                UpdatePassword(metaData, key, secretKeyType);
            }
            return true;
        }
    } else {
        return CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, key, meta.area, meta.user);
    }
    return false;
}

} // namespace OHOS::DistributedKv
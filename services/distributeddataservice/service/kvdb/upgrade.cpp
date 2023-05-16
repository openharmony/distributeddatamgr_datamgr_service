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
#define LOG_TAG "Upgrade"
#include "upgrade.h"

#include <chrono>
#include <cinttypes>

#include "crypto_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "store_cache.h"
#include "accesstoken_kit.h"
#include "directory_manager.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using system_clock = std::chrono::system_clock;
using DMAdapter = DistributedData::DeviceManagerAdapter;
using DBKey = DistributedDB::Key;

Upgrade &Upgrade::GetInstance()
{
    static Upgrade upgrade;
    return upgrade;
}

Upgrade::DBStatus Upgrade::UpdateStore(const StoreMeta &old, const StoreMeta &meta, const std::vector<uint8_t> &pwd)
{
    if (old.version < StoreMeta::UUID_CHANGED_TAG && old.storeType == DEVICE_COLLABORATION) {
        auto upStatus = Upgrade::GetInstance().UpdateUuid(old, meta, pwd);
        if (upStatus != DBStatus::OK) {
            return DBStatus::DB_ERROR;
        }
    }

    if (old.dataDir == meta.dataDir) {
        return DBStatus::OK;
    }

    if (!exporter_ || !cleaner_) {
        return DBStatus::NOT_SUPPORT;
    }

    DBPassword password;
    auto backupFile = exporter_(old, password);
    if (backupFile.empty()) {
        return DBStatus::NOT_FOUND;
    }

    auto kvStore = GetDBStore(meta, pwd);
    if (kvStore == nullptr) {
        return DBStatus::DB_ERROR;
    }

    cleaner_(old);
    return DBStatus::OK;
}

Upgrade::DBStatus Upgrade::ExportStore(const StoreMeta &old, const StoreMeta &meta)
{
    if (old.dataDir == meta.dataDir) {
        return DBStatus::OK;
    }

    if (!exporter_) {
        return DBStatus::NOT_SUPPORT;
    }

    DBPassword password;
    auto backupFile = exporter_(old, password);
    if (backupFile.empty()) {
        return DBStatus::NOT_FOUND;
    }
    return DBStatus::OK;
}

void Upgrade::UpdatePassword(const StoreMeta &meta, const std::vector<uint8_t> &password)
{
    if (!meta.isEncrypt) {
        return ;
    }

    SecretKeyMetaData secretKey;
    secretKey.storeType = meta.storeType;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(password);
    auto time = system_clock::to_time_t(system_clock::now());
    secretKey.time = { reinterpret_cast<uint8_t *>(&time), reinterpret_cast<uint8_t *>(&time) + sizeof(time) };
    MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), secretKey, true);
}

Upgrade::DBStatus Upgrade::UpdateUuid(const StoreMeta &old, const StoreMeta &meta, const std::vector<uint8_t> &pwd)
{
    auto kvStore = GetDBStore(meta, pwd);
    if (kvStore == nullptr) {
        return DBStatus::DB_ERROR;
    }
    kvStore->RemoveDeviceData();
    auto uuid = GetEncryptedUuidByMeta(meta);
    auto dbStatus = kvStore->UpdateKey([uuid](const DBKey &originKey, DBKey &newKey) {
        newKey = originKey;
        errno_t err = EOK;
        err = memcpy_s(newKey.data(), newKey.size(), uuid.data(), uuid.size());
        if (err != EOK) {
            ZLOGE("memcpy_s failed, err:%{public}d", err);
        }
    });
    if (dbStatus != DBStatus::OK) {
        ZLOGE("fail to update Uuid, status:%{public}d", dbStatus);
    }
    return dbStatus;
}

bool Upgrade::RegisterExporter(uint32_t version, Exporter exporter)
{
    (void)version;
    exporter_ = std::move(exporter);
    return exporter_ != nullptr;
}

bool Upgrade::RegisterCleaner(uint32_t version, Cleaner cleaner)
{
    (void)version;
    cleaner_ = std::move(cleaner);
    return cleaner_ != nullptr;
}

Upgrade::AutoStore Upgrade::GetDBStore(const StoreMeta &meta, const std::vector<uint8_t> &pwd)
{
    DBManager manager(meta.appId, meta.user, meta.instanceId);
    manager.SetKvStoreConfig({ DirectoryManager::GetInstance().GetStorePath(meta) });
    auto release = [&manager](DBStore *store) { manager.CloseKvStore(store); };
    DBPassword password;
    password.SetValue(pwd.data(), pwd.size());
    AutoStore dbStore(nullptr, release);
    manager.GetKvStore(meta.storeId, StoreCache::GetDBOption(meta, password),
        [&dbStore](auto dbStatus, auto *tmpStore) {
            dbStore.reset(tmpStore);
        });
    return dbStore;
}

std::string Upgrade::GetEncryptedUuidByMeta(const StoreMeta &meta)
{
    std::string keyUuid = meta.appId + meta.deviceId;
    auto pair = calcUuid_.Find(keyUuid);
    if (pair.first) {
        return pair.second;
    }
    std::string uuid;
    if (OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(meta.tokenId) ==
        OHOS::Security::AccessToken::TOKEN_HAP) {
        uuid = DMAdapter::GetInstance().CalcClientUuid(meta.appId, meta.deviceId);
        calcUuid_.Insert(keyUuid, uuid);
        return uuid;
    }
    uuid = DMAdapter::GetInstance().CalcClientUuid(" ", meta.deviceId);
    calcUuid_.Insert(keyUuid, uuid);
    return uuid;
}
}
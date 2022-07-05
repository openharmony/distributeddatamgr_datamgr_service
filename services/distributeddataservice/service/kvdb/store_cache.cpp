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
#define LOG_TAG "StoreCache"
#include "store_cache.h"
#include "account/account_delegate.h"
#include "crypto_manager.h"
#include "directory_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "types.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
constexpr int64_t StoreCache::INTERVAL;
constexpr size_t StoreCache::TIME_TASK_NUM;
StoreCache::DBStore *StoreCache::GetStore(const StoreMetaData &data, std::shared_ptr<Observers> observers,
    DBStatus &status)
{
    DBStore *store = nullptr;
    status = DBStatus::NOT_FOUND;
    stores_.Compute(data.tokenId, [&](const auto &key, std::map<std::string, DBStoreDelegate> &stores) {
        auto it = stores.find(data.storeId);
        if (it != stores.end()) {
            it->second.SetObservers(observers);
            store = it->second;
            return true;
        }

        DBManager manager(data.appId, data.user);
        manager.SetKvStoreConfig({ DirectoryManager::GetInstance().GetStorePath(data) });
        manager.GetKvStore(data.storeId, GetDBOption(data, GetDBPassword(data)),
            [&status, &store](auto dbStatus, auto *tmpStore) {
                status = dbStatus;
                store = tmpStore;
            });

        if (store == nullptr) {
            return !stores.empty();
        }

        if (data.isAutoSync) {
            bool autoSync = true;
            DistributedDB::PragmaData data = static_cast<DistributedDB::PragmaData>(&autoSync);
            auto syncStatus = store->Pragma(DistributedDB::PragmaCmd::AUTO_SYNC, data);
            if (syncStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("auto sync status:%{public}d", static_cast<int>(syncStatus));
            }
        }

        stores.emplace(std::piecewise_construct, std::forward_as_tuple(data.storeId),
            std::forward_as_tuple(store, observers));
        return !stores.empty();
    });

    scheduler_.At(std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL),
        std::bind(&StoreCache::GarbageCollect, this));

    return store;
}

void StoreCache::CloseStore(uint32_t tokenId, const std::string &storeId)
{
    stores_.ComputeIfPresent(tokenId, [&storeId](auto &key, std::map<std::string, DBStoreDelegate> &delegates) {
        DBManager manager("", "");
        auto it = delegates.find(storeId);
        if (it != delegates.end()) {
            it->second.Close(manager);
            delegates.erase(it);
        }
        return !delegates.empty();
    });
}

void StoreCache::CloseExcept(const std::set<int32_t> &users)
{
    DBManager manager("", "");
    stores_.EraseIf([&manager, &users](const auto &tokenId, std::map<std::string, DBStoreDelegate> &delegates) {
        auto userId = AccountDelegate::GetInstance()->GetUserByToken(tokenId);
        if (users.count(userId) != 0) {
            return delegates.empty();
        }
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the kv store is BUSY we wait more INTERVAL minutes again
            if (!it->second.Close(manager)) {
                ++it;
            } else {
                it = delegates.erase(it);
            }
        }
        return delegates.empty();
    });
}

void StoreCache::SetObserver(uint32_t tokenId, const std::string &storeId, std::shared_ptr<Observers> observers)
{
    stores_.ComputeIfPresent(tokenId, [&storeId, &observers](auto &key, auto &stores) {
        ZLOGD("tokenId:0x%{public}x storeId:%{public}s observers:%{public}zu", key, storeId.c_str(),
            observers ? observers->size() : size_t(0));
        auto it = stores.find(storeId);
        if (it != stores.end()) {
            it->second.SetObservers(observers);
        }
        return true;
    });
}

void StoreCache::GarbageCollect()
{
    DBManager manager("", "");
    auto current = std::chrono::system_clock::now();
    stores_.EraseIf([&manager, &current](auto &key, std::map<std::string, DBStoreDelegate> &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the kv store is BUSY we wait more INTERVAL minutes again
            if ((it->second < current) || !it->second.Close(manager)) {
                ++it;
            } else {
                it = delegates.erase(it);
            }
        }
        return delegates.empty();
    });

    if (!stores_.Empty()) {
        scheduler_.At(current + std::chrono::minutes(INTERVAL), std::bind(&StoreCache::GarbageCollect, this));
    }
}

StoreCache::DBOption StoreCache::GetDBOption(const StoreMetaData &data, const DBPassword &password)
{
    DBOption dbOption;
    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = false;
    dbOption.isMemoryDb = false;
    dbOption.isEncryptedDb = data.isEncrypt;
    if (data.isEncrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = password;
    }

    if (data.storeType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (data.storeType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    }

    dbOption.schema = data.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = GetDBSecurity(data.securityLevel);
    return dbOption;
}

StoreCache::DBSecurity StoreCache::GetDBSecurity(int32_t secLevel)
{
    if (secLevel < SecurityLevel::NO_LABEL || secLevel > SecurityLevel::S4) {
        return { DistributedDB::NOT_SET, DistributedDB::ECE };
    }
    if (secLevel == SecurityLevel::S3) {
        return { DistributedDB::S3, DistributedDB::SECE };
    }
    if (secLevel == SecurityLevel::S4) {
        return { DistributedDB::S4, DistributedDB::ECE };
    }
    return { secLevel, DistributedDB::ECE };
}

StoreCache::DBPassword StoreCache::GetDBPassword(const StoreMetaData &data)
{
    DBPassword dbPassword;
    if (!data.isEncrypt) {
        return dbPassword;
    }

    SecretKeyMetaData secretKey;
    secretKey.storeType = data.storeType;
    auto storeKey = data.GetSecretKey();
    MetaDataManager::GetInstance().LoadMeta(storeKey, secretKey, true);
    std::vector<uint8_t> password;
    CryptoManager::GetInstance().Decrypt(secretKey.sKey, password);
    dbPassword.SetValue(password.data(), password.size());
    password.assign(password.size(), 0);
    return dbPassword;
}

StoreCache::DBStoreDelegate::DBStoreDelegate(DBStore *delegate, std::shared_ptr<Observers> observers)
    : delegate_(delegate)
{
    time_ = std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL);
    SetObservers(std::move(observers));
}

StoreCache::DBStoreDelegate::~DBStoreDelegate()
{
    if (delegate_ != nullptr) {
        delegate_->UnRegisterObserver(this);
    }
    DBManager manager("", "");
    manager.CloseKvStore(delegate_);
    delegate_ = nullptr;
}

StoreCache::DBStoreDelegate::operator DBStore *()
{
    time_ = std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL);
    return delegate_;
}

bool StoreCache::DBStoreDelegate::operator<(const Time &time) const
{
    return time_ < time;
}

bool StoreCache::DBStoreDelegate::Close(DBManager &manager)
{
    if (delegate_ != nullptr) {
        delegate_->UnRegisterObserver(this);
    }

    auto status = manager.CloseKvStore(delegate_);
    if (status == DBStatus::BUSY) {
        return false;
    }
    delegate_ = nullptr;
    return true;
}

void StoreCache::DBStoreDelegate::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    if (observers_ == nullptr || delegate_ == nullptr) {
        ZLOGE("already closed");
        return;
    }

    time_ = std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL);
    auto observers = observers_;
    std::vector<uint8_t> key;
    auto inserts = Convert(data.GetEntriesInserted());
    auto updates = Convert(data.GetEntriesUpdated());
    auto deletes = Convert(data.GetEntriesDeleted());
    ZLOGD("C:%{public}zu U:%{public}zu D:%{public}zu storeId:%{public}s", inserts.size(), updates.size(),
        deletes.size(), delegate_->GetStoreId().c_str());
    ChangeNotification change(std::move(inserts), std::move(updates), std::move(deletes), {}, false);
    for (auto &observer : *observers) {
        observer->OnChange(change);
    }
}

void StoreCache::DBStoreDelegate::SetObservers(std::shared_ptr<Observers> observers)
{
    if (observers_ == observers || delegate_ == nullptr) {
        return;
    }

    observers_ = observers;

    if (observers_ != nullptr && !observers_->empty()) {
        ZLOGD("storeId:%{public}s observers:%{public}zu", delegate_->GetStoreId().c_str(), observers_->size());
        delegate_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_FOREIGN, this);
    }
}

std::vector<Entry> StoreCache::DBStoreDelegate::Convert(const std::list<DBEntry> &dbEntries)
{
    std::vector<Entry> entries;
    for (const auto &entry : dbEntries) {
        Entry tmpEntry;
        tmpEntry.key = entry.key;
        tmpEntry.value = entry.value;
        entries.push_back(tmpEntry);
    }
    return entries;
}
}; // namespace OHOS::DistributedKv

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
#include "device_matrix.h"
#include "directory_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "types.h"
#include "utils/anonymous.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
constexpr int64_t StoreCache::INTERVAL;
constexpr size_t StoreCache::TIME_TASK_NUM;
StoreCache::Store StoreCache::GetStore(const StoreMetaData &data, std::shared_ptr<Observers> observers,
    DBStatus &status)
{
    Store store = nullptr;
    status = DBStatus::NOT_FOUND;
    stores_.Compute(data.tokenId, [&](const auto &key, std::map<std::string, DBStoreDelegate> &stores) {
        auto it = stores.find(data.storeId);
        if (it != stores.end()) {
            it->second.SetObservers(observers);
            store = it->second;
            return true;
        }

        DBStore *dbStore = nullptr;
        DBManager manager(data.appId, data.user, data.instanceId);
        manager.SetKvStoreConfig({ DirectoryManager::GetInstance().GetStorePath(data) });
        manager.GetKvStore(data.storeId, GetDBOption(data, GetDBPassword(data)),
            [&status, &dbStore](auto dbStatus, auto *tmpStore) {
                status = dbStatus;
                dbStore = tmpStore;
            });

        if (dbStore == nullptr) {
            return !stores.empty();
        }

        if (data.isAutoSync) {
            auto code = DeviceMatrix::GetInstance().GetCode(data);
            dbStore->SetRemotePushFinishedNotify([code](const DistributedDB::RemotePushNotifyInfo &info) {
                DeviceMatrix::GetInstance().OnExchanged(info.deviceId, code, true);
            });
        }

        auto result = stores.emplace(std::piecewise_construct, std::forward_as_tuple(data.storeId),
            std::forward_as_tuple(dbStore, observers));
        store = result.first->second;
        return !stores.empty();
    });

    scheduler_.At(std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL),
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
        ZLOGD("tokenId:0x%{public}x storeId:%{public}s observers:%{public}zu", key,
            Anonymous::Change(storeId).c_str(), observers ? observers->size() : size_t(0));
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
    auto current = std::chrono::steady_clock::now();
    stores_.EraseIf([&manager, &current](auto &key, std::map<std::string, DBStoreDelegate> &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the kv store is BUSY we wait more INTERVAL minutes again
            if ((it->second < current) && it->second.Close(manager)) {
                it = delegates.erase(it);
            } else {
                ++it;
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
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
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

StoreCache::DBStoreDelegate::operator std::shared_ptr<DBStore> ()
{
    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    mutex_.lock_shared();
    if (delegate_ == nullptr) {
        mutex_.unlock_shared();
        return nullptr;
    }
    return std::shared_ptr<DBStore>(delegate_, [this](DBStore *) { mutex_.unlock_shared();});
}

bool StoreCache::DBStoreDelegate::operator<(const Time &time) const
{
    return time_ < time;
}

bool StoreCache::DBStoreDelegate::Close(DBManager &manager)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
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

    time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    auto observers = observers_;
    std::vector<uint8_t> key;
    auto inserts = Convert(data.GetEntriesInserted());
    auto updates = Convert(data.GetEntriesUpdated());
    auto deletes = Convert(data.GetEntriesDeleted());
    ZLOGD("C:%{public}zu U:%{public}zu D:%{public}zu storeId:%{public}s", inserts.size(), updates.size(),
        deletes.size(), Anonymous::Change(delegate_->GetStoreId()).c_str());
    ChangeNotification change(std::move(inserts), std::move(updates), std::move(deletes), {}, false);
    for (auto &observer : *observers) {
        if (observer == nullptr) {
            continue;
        }
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
        ZLOGD("storeId:%{public}s observers:%{public}zu", Anonymous::Change(delegate_->GetStoreId()).c_str(),
            observers_->size());
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

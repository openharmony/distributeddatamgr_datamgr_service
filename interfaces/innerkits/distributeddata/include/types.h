/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_KVSTORE_TYPES_H
#define DISTRIBUTED_KVSTORE_TYPES_H

#include <cstdint>
#include <string>
#include <vector>
#include "store_errno.h"
#include "blob.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
// key set by client, can be any non-empty bytes array, and less than 1024 size.
using Key = OHOS::DistributedKv::Blob;

// value set by client, can be any bytes array.
using Value = OHOS::DistributedKv::Blob;

// user identifier from user-account
struct UserId {
    std::string userId;
};

// app identifier from Bms
struct API_EXPORT AppId {
    std::string appId;

    // support AppId convert to std::string
    operator std::string &() noexcept
    {
        return appId;
    }
    // support AppId convert to const std::string
    operator const std::string &() const noexcept
    {
        return appId;
    }

    inline bool IsValid() const
    {
        if (appId.empty() || appId.size() > MAX_APP_ID_LEN) {
            return false;
        }
        int count = 0;
        auto iter = std::find_if_not(appId.begin(), appId.end(),
            [&count](char c) {
            count = (c == SEPARATOR_CHAR) ? (count + 1) : (count >= SEPARATOR_COUNT ? count : 0);
            return (std::isprint(c) && c != '/');
        });

        return (iter == appId.end()) && (count < SEPARATOR_COUNT);
    }
private:
    static constexpr int MAX_APP_ID_LEN = 256;
    static constexpr int SEPARATOR_COUNT = 3;
    static constexpr char SEPARATOR_CHAR = '#';
};

// kvstore name set by client by calling GetKvStore,
// storeId len must be less or equal than 256,
// and can not be empty and all space.
struct API_EXPORT StoreId {
    std::string storeId;

    // support StoreId convert to std::string
    operator std::string &() noexcept
    {
        return storeId;
    }
    // support StoreId convert to const std::string
    operator const std::string &() const noexcept
    {
        return storeId;
    }

    inline bool IsValid() const
    {
        if (storeId.empty() || storeId.size() > MAX_STORE_ID_LEN) {
            return false;
        }
        auto iter = std::find_if_not(storeId.begin(), storeId.end(),
            [](char c) { return (std::isdigit(c) || std::isalpha(c) || c == '_'); });
        return (iter == storeId.end());
    }
private:
    static constexpr int MAX_STORE_ID_LEN = 128;
};

struct KvStoreTuple {
    std::string userId;
    std::string appId;
    std::string storeId;
};

struct AppThreadInfo {
    std::int32_t pid;
    std::int32_t uid;
};

enum class SubscribeType {
    DEFAULT = 0, // default let bms delete
    SUBSCRIBE_TYPE_LOCAL = 1, // local changes of syncable kv store
    SUBSCRIBE_TYPE_REMOTE = 2, // synced data changes from remote devices
    SUBSCRIBE_TYPE_ALL = 3, // both local changes and synced data changes
};

struct Entry : public virtual Parcelable {
    Key key;
    Value value;
    // Write a parcelable object to the given parcel.
    // The object position is saved into Parcel if set savePosition to
    // true, and this intends to use in kernel data transaction.
    // Returns size being written on success or zero if any error occur.
    bool Marshalling(Parcel &parcel) const override
    {
        if (!parcel.WriteParcelable(&key)) {
            return false;
        }
        if (!parcel.WriteParcelable(&value)) {
            return false;
        }
        return true;
    }

    // Read data from the given parcel into this parcelable object.
    // Returns size being read on success or zero if any error occur.
    static Entry *Unmarshalling(Parcel &parcel)
    {
        Entry *entry = new(std::nothrow) Entry;
        if (entry == nullptr) {
            return entry;
        }

        bool noError = true;
        sptr<Key> keyTmp = parcel.ReadParcelable<Key>();
        if (keyTmp != nullptr) {
            entry->key = *keyTmp;
        } else {
            noError = false;
        }

        sptr<Value> valueTmp = parcel.ReadParcelable<Value>();
        if (valueTmp != nullptr) {
            entry->value = *valueTmp;
        } else {
            noError = false;
        }

        if (!noError) {
            delete entry;
            entry = nullptr;
        }
        return entry;
    }

    API_EXPORT virtual ~Entry() {}
};

enum class SyncPolicy {
    LOW,
    MEDIUM,
    HIGH,
    HIGHTEST,
};

enum class SyncMode {
    PULL,
    PUSH,
    PUSH_PULL,
};

enum KvStoreType : int32_t {
    DEVICE_COLLABORATION,
    SINGLE_VERSION,
    MULTI_VERSION,
    INVALID_TYPE,
};

enum SecurityLevel : int32_t {
    NO_LABEL,
    S0,
    S1,
    S2,
    S3_EX,
    S3,
    S4,
};

enum Area : int32_t {
    EL0,
    EL1,
    EL2,
    EL3,
    EL4
};

enum class KvControlCmd {
    SET_SYNC_PARAM = 1,
    GET_SYNC_PARAM,
};

using KvParam = OHOS::DistributedKv::Blob;

struct KvSyncParam {
    uint32_t allowedDelayMs { 0 };
};

enum class DeviceChangeType {
    DEVICE_OFFLINE = 0,
    DEVICE_ONLINE = 1,
};

struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string deviceType;
};

enum class DeviceFilterStrategy {
    FILTER = 0,
    NO_FILTER = 1,
};

struct Options {
    bool createIfMissing = true;
    bool encrypt = false;
    bool persistent = true;
    bool backup = true;
    bool autoSync = true;
    int32_t securityLevel = NO_LABEL;
    int32_t area = EL1;
    SyncPolicy syncPolicy = SyncPolicy::HIGH;
    KvStoreType kvStoreType = DEVICE_COLLABORATION;
    bool syncable = true; // let bms delete first
    std::string schema = "";
    bool dataOwnership = true; // true indicates the ownership of distributed data is DEVICE, otherwise, ACCOUNT

    inline bool IsValidType() const
    {
        return kvStoreType == KvStoreType::DEVICE_COLLABORATION || kvStoreType == KvStoreType::SINGLE_VERSION;
    }
};

template<typename T>
std::vector<uint8_t> TransferTypeToByteArray(const T &t)
{
    return std::vector<uint8_t>(reinterpret_cast<uint8_t *>(const_cast<T *>(&t)),
                                reinterpret_cast<uint8_t *>(const_cast<T *>(&t)) + sizeof(T));
}

template<typename T>
T TransferByteArrayToType(const std::vector<uint8_t> &blob)
{
    // replace assert to HILOG_FATAL when HILOG_FATAL is ok.
    if (blob.size() != sizeof(T) || blob.size() == 0) {
        constexpr int tSize = sizeof(T);
        uint8_t tContent[tSize] = { 0 };
        return *reinterpret_cast<T *>(tContent);
    }
    return *reinterpret_cast<T *>(const_cast<uint8_t *>(&blob[0]));
}
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // DISTRIBUTED_KVSTORE_TYPES_H

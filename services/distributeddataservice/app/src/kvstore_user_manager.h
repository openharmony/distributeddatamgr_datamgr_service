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

#ifndef KV_STORE_USER_MANAGER_H
#define KV_STORE_USER_MANAGER_H

#include <map>
#include <mutex>
#include "kvstore_app_manager.h"
#include "types.h"
#include "metadata/store_meta_data.h"

namespace OHOS {
namespace DistributedKv {
class KvStoreUserManager {
public:
    using StoreMetaData = DistributedData::StoreMetaData;
    explicit KvStoreUserManager(const std::string &userId);

    virtual ~KvStoreUserManager();

    template<typename T>
    Status GetKvStore(
        const Options &options, const StoreMetaData &metaData, const std::vector<uint8_t> &cipherKey, sptr<T> &kvStore)
    {
        std::lock_guard<decltype(appMutex_)> lg(appMutex_);
        auto it = appMap_.find(metaData.bundleName);
        if (it == appMap_.end()) {
            auto result = appMap_.emplace(std::piecewise_construct, std::forward_as_tuple(metaData.bundleName),
                std::forward_as_tuple(metaData.bundleName, metaData.uid, metaData.tokenId));
            if (result.second) {
                it = result.first;
            }
        }
        if (it == appMap_.end()) {
            kvStore = nullptr;
            return Status::ERROR;
        }
        return (it->second).GetKvStore(options, metaData, cipherKey, kvStore);
    }

    Status CloseKvStore(const std::string &appId, const std::string &storeId);

    Status CloseAllKvStore(const std::string &appId);

    void CloseAllKvStore();

    Status DeleteKvStore(const std::string &bundleName, pid_t uid, uint32_t token, const std::string &storeId);

    void DeleteAllKvStore();

    void Dump(int fd) const;
    void DumpUserInfo(int fd) const;
    void DumpAppInfo(int fd, const std::string &appId) const;
    void DumpStoreInfo(int fd, const std::string &storeId) const;

    bool IsStoreOpened(const std::string &appId, const std::string &storeId);
    void SetCompatibleIdentify(const std::string &deviceId) const;

private:
    mutable std::recursive_mutex appMutex_;
    std::map<std::string, KvStoreAppManager> appMap_;
    std::string userId_;
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // KV_STORE_USER_MANAGER_H

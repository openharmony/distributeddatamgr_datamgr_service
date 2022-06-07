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

#ifndef KV_STORE_APP_MANAGER_H
#define KV_STORE_APP_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include "flowctrl_manager/kvstore_flowctrl_manager.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvstore_meta_manager.h"
#include "metadata/store_meta_data.h"
#include "nocopyable.h"
#include "single_kvstore_impl.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class KvStoreAppManager {
public:
    enum PathType {
        PATH_DE,
        PATH_CE,
        PATH_TYPE_MAX
    };
    using StoreMetaData = DistributedData::StoreMetaData;

    KvStoreAppManager(const std::string &bundleName, pid_t uid, uint32_t token);

    virtual ~KvStoreAppManager();

    Status GetKvStore(const Options &options, const StoreMetaData &metaData, const std::vector<uint8_t> &cipherKey,
        sptr<SingleKvStoreImpl> &kvStore);

    Status CloseKvStore(const std::string &storeId);

    Status CloseAllKvStore();

    Status DeleteKvStore(const std::string &storeId);

    Status DeleteAllKvStore();

    static Status InitNbDbOption(const Options &options, const std::vector<uint8_t> &cipherKey,
                                 DistributedDB::KvStoreNbDelegate::Option &dbOption);

    static std::string GetDataStoragePath(const std::string &userId, const std::string &bundleName,
                                          PathType type);

    static PathType ConvertPathType(const StoreMetaData &metaData);

    static std::string GetDbDir(const StoreMetaData &metaData);

    void Dump(int fd) const;
    void DumpUserInfo(int fd) const;
    void DumpAppInfo(int fd) const;
    void DumpStoreInfo(int fd, const std::string &storeId) const;

    static DistributedDB::SecurityOption ConvertSecurity(int securityLevel);

    size_t GetTotalKvStoreNum() const;

    bool IsStoreOpened(const std::string &storeId) const;
    void SetCompatibleIdentify(const std::string &deviceId) const;

private:
    DISALLOW_COPY_AND_MOVE(KvStoreAppManager);
    Status ConvertErrorStatus(DistributedDB::DBStatus dbStatus, bool createIfMissing);
    DistributedDB::KvStoreDelegateManager *GetDelegateManager(PathType type);
    DistributedDB::KvStoreDelegateManager *SwitchDelegateManager(PathType type,
        DistributedDB::KvStoreDelegateManager *delegateManager);
    Status CloseKvStore(const std::string &storeId, PathType type);
    Status CloseAllKvStore(PathType type);
    Status DeleteKvStore(const std::string &storeId, PathType type);
    Status DeleteAllKvStore(PathType type);
    Status MigrateAllKvStore(const std::string &harmonyAccountId, PathType type);
    std::mutex storeMutex_ {};
    std::map<std::string, sptr<SingleKvStoreImpl>> singleStores_[PATH_TYPE_MAX] {};
    std::string userId_ {};
    std::string bundleName_ {};
    std::string deviceAccountId_ {};
    std::string trueAppId_ {};
    pid_t uid_;
    uint32_t token_;
    std::mutex delegateMutex_ {};
    DistributedDB::KvStoreDelegateManager *delegateManagers_[PATH_TYPE_MAX] {nullptr, nullptr};
    KvStoreFlowCtrlManager flowCtrl_;
    static inline const int BURST_CAPACITY = 50;
    static inline const int SUSTAINED_CAPACITY = 500;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // KV_STORE_APP_MANAGER_H

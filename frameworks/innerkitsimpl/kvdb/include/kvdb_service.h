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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#include <iremote_broker.h>

#include <memory>

#include "kvstore_death_recipient.h"
#include "single_kvstore.h"
#include "types.h"
#include "visibility.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBService : public IRemoteBroker {
public:
    using SingleKVStore = SingleKvStore;

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.KVDBService");

    API_EXPORT KVDBService() = default;

    API_EXPORT virtual ~KVDBService() = default;

    virtual Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) = 0;

    virtual Status Delete(const std::string &path, const AppId &appId, const StoreId &storeId) = 0;

    virtual Status Sync(const std::vector<std::string> &devices, const AppId &appId, const StoreId &storeId) = 0;

protected:
    enum TransId : int32_t {
        TRANS_HEAD,
        TRANS_GET_STORE_IDS = TRANS_HEAD,
        TRANS_BEFORE_CREATE,
        TRANS_AFTER_CREATE,
        TRANS_DELETE,
        TRANS_SYNC,
        TRANS_BUTT,
    };

    virtual Status BeforeCreate(const Options &options, const AppId &appId, const StoreId &storeId) = 0;

    virtual Status AfterCreate(
        const Options &options, const AppId &appId, const StoreId &storeId, const std::vector<uint8_t> &password) = 0;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H

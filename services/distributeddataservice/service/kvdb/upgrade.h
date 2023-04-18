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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_UPGRADE_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_UPGRADE_H
#include <functional>
#include <memory>
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "metadata/store_meta_data.h"
#include "types.h"
#include "concurrent_map.h"

namespace OHOS::DistributedKv {
class Upgrade {
public:
    using StoreMeta = DistributedData::StoreMetaData;
    using DBPassword = DistributedDB::CipherPassword;
    using DBStatus = DistributedDB::DBStatus;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using Exporter = std::function<std::string(const StoreMeta &, DBPassword &)>;
    using Cleaner = std::function<Status(const StoreMeta &)>;
    API_EXPORT static Upgrade &GetInstance();
    API_EXPORT bool RegisterExporter(uint32_t version, Exporter exporter);
    API_EXPORT bool RegisterCleaner(uint32_t version, Cleaner cleaner);

    DBStatus UpdateStore(const StoreMeta &old, const StoreMeta &metaData, const std::vector<uint8_t> &pwd);
    DBStatus ExportStore(const StoreMeta &old, const StoreMeta &meta);
    void UpdatePassword(const StoreMeta &meta, const std::vector<uint8_t> &password);
    DBStatus UpdateUuid(const StoreMeta &old, const StoreMeta &meta, const std::vector<uint8_t> &pwd);
    API_EXPORT std::string GetEncryptedUuidByMeta(const StoreMeta &meta);

private:
    using AutoStore = std::unique_ptr<DBStore, std::function<void(DBStore *)>>;
    AutoStore GetDBStore(const StoreMeta &meta, const std::vector<uint8_t> &pwd);
    ConcurrentMap<std::string, std::string> calcUuid_;

    Exporter exporter_;
    Cleaner cleaner_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_UPGRADE_H

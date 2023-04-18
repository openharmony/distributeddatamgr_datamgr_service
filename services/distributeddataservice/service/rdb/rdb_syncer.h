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

#ifndef DISTRIBUTED_RDB_SYNCER_H
#define DISTRIBUTED_RDB_SYNCER_H

#include <mutex>
#include <string>

#include "metadata/store_meta_data.h"
#include "rdb_notifier.h"
#include "rdb_store_observer_impl.h"
#include "rdb_types.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "metadata/secret_key_meta_data.h"
namespace OHOS::DistributedRdb {
class RdbSyncer {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    RdbSyncer(const RdbSyncerParam& param, RdbStoreObserverImpl* observer);
    ~RdbSyncer() noexcept;

    int32_t Init(pid_t pid, pid_t uid, uint32_t token, const std::string &writePermission = "",
        const std::string &readPermission = "");

    pid_t GetPid() const;

    void SetTimerId(uint32_t timerId);

    uint32_t GetTimerId() const;

    std::string GetStoreId() const;

    std::string GetIdentifier() const;

    int32_t SetDistributedTables(const std::vector<std::string>& tables);

    int32_t DoSync(const SyncOption& option, const RdbPredicates& predicates, SyncResult& result);

    int32_t DoAsync(const SyncOption& option, const RdbPredicates& predicates, const SyncCallback& callback);

    int32_t RemoteQuery(const std::string& device, const std::string& sql,
                        const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet);

    int32_t DestroyMetaData(StoreMetaData &meta);
    static std::string RemoveSuffix(const std::string& name);

    static int32_t GetInstIndex(uint32_t tokenId, const std::string &bundleName);

    static bool GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password);

private:
    std::string GetUserId() const;

    std::string GetBundleName() const;

    std::string GetAppId() const;

    int32_t CreateMetaData(StoreMetaData &meta, StoreMetaData &old);
    void FillMetaData(StoreMetaData &meta);
    int32_t InitDBDelegate(const StoreMetaData &meta);
    bool SetSecretKey(const StoreMetaData &meta);

    DistributedDB::RelationalStoreDelegate* GetDelegate();

    std::mutex mutex_;
    DistributedDB::RelationalStoreManager* manager_ {};
    DistributedDB::RelationalStoreDelegate* delegate_ {};
    RdbSyncerParam param_;
    RdbStoreObserverImpl *observer_ {};
    pid_t pid_ {};
    pid_t uid_ {};
    uint32_t token_ {};
    uint32_t timerId_ {};

    static std::vector<std::string> GetConnectDevices();
    static std::vector<std::string> NetworkIdToUUID(const std::vector<std::string>& networkIds);

    static void HandleSyncStatus(const std::map<std::string, std::vector<DistributedDB::TableStatus>>& SyncStatus,
                                 SyncResult& result);
    static DistributedDB::Query MakeQuery(const RdbPredicates& predicates);
    static void EqualTo(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static void NotEqualTo(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static void And(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static void Or(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static void OrderBy(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static void Limit(const RdbPredicateOperation& operation, DistributedDB::Query& query);

    using PredicateHandle = void(*)(const RdbPredicateOperation& operation, DistributedDB::Query& query);
    static inline PredicateHandle HANDLES[OPERATOR_MAX] = {
        [EQUAL_TO] = &RdbSyncer::EqualTo,
        [NOT_EQUAL_TO] = &RdbSyncer::NotEqualTo,
        [AND] = &RdbSyncer::And,
        [OR] = &RdbSyncer::Or,
        [ORDER_BY] = &RdbSyncer::OrderBy,
        [LIMIT] = &RdbSyncer::Limit,
    };

    static constexpr int DECIMAL_BASE = 10;
    static constexpr uint64_t REMOTE_QUERY_TIME_OUT = 30 * 1000;
};
} // namespace OHOS::DistributedRdb
#endif

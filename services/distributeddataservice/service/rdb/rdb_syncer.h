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
#include <traits.h>

#include "iremote_object.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "rdb_service.h"
#include "rdb_store_observer_impl.h"
#include "rdb_types.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "store/general_value.h"
namespace OHOS::DistributedRdb {
class RdbSyncer {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using GenDetails = OHOS::DistributedData::GenDetails;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    using Option = DistributedRdb::RdbService::Option;
    RdbSyncer(const RdbSyncerParam& param, RdbStoreObserverImpl* observer);
    ~RdbSyncer() noexcept;

    int32_t Init(pid_t pid, pid_t uid, uint32_t token, const StoreMetaData &meta);

    pid_t GetPid() const;

    void SetTimerId(uint64_t timerId);

    uint64_t GetTimerId() const;

    std::string GetStoreId() const;

    std::string GetIdentifier() const;

    int32_t SetDistributedTables(const std::vector<std::string> &tables, int32_t type);

    int32_t DoSync(const Option &option, const PredicatesMemo &predicates, const AsyncDetail &async);

    int32_t RemoteQuery(const std::string& device, const std::string& sql,
                        const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet);

    int32_t RemoveDeviceData();

    static std::string RemoveSuffix(const std::string& name);

    static std::pair<int32_t, int32_t> GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName);

    static bool GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password);

    static DistributedDB::Query MakeQuery(const PredicatesMemo &predicates);

private:
    std::string GetUserId() const;

    std::string GetBundleName() const;

    std::string GetAppId() const;

    int32_t InitDBDelegate(const StoreMetaData &meta);

    DistributedDB::RelationalStoreDelegate* GetDelegate();

    std::mutex mutex_;
    DistributedDB::RelationalStoreManager* manager_ {};
    DistributedDB::RelationalStoreDelegate* delegate_ {};
    RdbSyncerParam param_;
    RdbStoreObserverImpl *observer_ {};
    pid_t pid_ {};
    pid_t uid_ {};
    uint32_t token_ {};
    uint64_t timerId_ {};

    static std::vector<std::string> GetConnectDevices();
    static std::vector<std::string> NetworkIdToUUID(const std::vector<std::string>& networkIds);

    static Details HandleSyncStatus(const std::map<std::string, std::vector<DistributedDB::TableStatus>> &syncStatus);
    static Details HandleGenDetails(const GenDetails &details);
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
    static constexpr int32_t WAIT_TIME = 30 * 1000;
};
} // namespace OHOS::DistributedRdb
#endif

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
#define LOG_TAG "RdbSyncer"
#include "rdb_syncer.h"

#include <chrono>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "checker/checker_manager.h"
#include "cloud/change_event.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "rdb_query.h"
#include "rdb_result_set_impl.h"
#include "store/general_store.h"
#include "types_export.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
using OHOS::DistributedKv::AccountDelegate;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::DistributedData;
using system_clock = std::chrono::system_clock;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

constexpr uint32_t ITERATE_TIMES = 10000;
namespace OHOS::DistributedRdb {
RdbSyncer::RdbSyncer(const RdbSyncerParam& param, RdbStoreObserverImpl* observer)
    : param_(param), observer_(observer)
{
    ZLOGI("construct %{public}s", Anonymous::Change(param_.storeName_).c_str());
}

RdbSyncer::~RdbSyncer() noexcept
{
    param_.password_.assign(param_.password_.size(), 0);
    ZLOGI("destroy %{public}s", Anonymous::Change(param_.storeName_).c_str());
    if ((manager_ != nullptr) && (delegate_ != nullptr)) {
        manager_->CloseStore(delegate_);
    }
    delete manager_;
    if (observer_ != nullptr) {
        delete observer_;
    }
}

void RdbSyncer::SetTimerId(uint64_t timerId)
{
    timerId_ = timerId;
}

uint64_t RdbSyncer::GetTimerId() const
{
    return timerId_;
}

pid_t RdbSyncer::GetPid() const
{
    return pid_;
}

std::string RdbSyncer::GetIdentifier() const
{
    return DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(GetUserId(), GetAppId(), GetStoreId());
}

std::string RdbSyncer::GetUserId() const
{
    return std::to_string(AccountDelegate::GetInstance()->GetUserByToken(token_));
}

std::string RdbSyncer::GetBundleName() const
{
    return param_.bundleName_;
}

std::string RdbSyncer::GetAppId() const
{
    return DistributedData::CheckerManager::GetInstance().GetAppId( { uid_, token_, param_.bundleName_ } );
}

std::string RdbSyncer::GetStoreId() const
{
    return RemoveSuffix(param_.storeName_);
}

int32_t RdbSyncer::Init(pid_t pid, pid_t uid, uint32_t token, const StoreMetaData &meta)
{
    ZLOGI("enter");
    pid_ = pid;
    uid_ = uid;
    token_ = token;

    if (InitDBDelegate(meta) != RDB_OK) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    ZLOGI("success");
    return RDB_OK;
}

bool RdbSyncer::GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password)
{
    if (!metaData.isEncrypt) {
        return true;
    }

    std::string key = metaData.GetSecretKey();
    DistributedData::SecretKeyMetaData secretKeyMeta;
    MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true);
    std::vector<uint8_t> decryptKey;
    CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
    if (password.SetValue(decryptKey.data(), decryptKey.size()) != DistributedDB::CipherPassword::OK) {
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
        ZLOGE("Set secret key value failed. len is (%{public}d)", int32_t(decryptKey.size()));
        return false;
    }
    std::fill(decryptKey.begin(), decryptKey.end(), 0);
    return true;
}

std::string RdbSyncer::RemoveSuffix(const std::string& name)
{
    std::string suffix(".db");
    auto pos = name.rfind(suffix);
    if (pos == std::string::npos || pos < name.length() - suffix.length()) {
        return name;
    }
    return std::string(name, 0, pos);
}

int32_t RdbSyncer::InitDBDelegate(const StoreMetaData &meta)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (manager_ == nullptr) {
        manager_ = new(std::nothrow) DistributedDB::RelationalStoreManager(meta.appId, meta.user, meta.instanceId);
    }
    if (manager_ == nullptr) {
        ZLOGE("malloc manager failed");
        return RDB_ERROR;
    }

    if (delegate_ == nullptr) {
        DistributedDB::RelationalStoreDelegate::Option option;
        if (meta.isEncrypt) {
            GetPassword(meta, option.passwd);
            option.isEncryptedDb = param_.isEncrypt_;
            option.iterateTimes = ITERATE_TIMES;
            option.cipher = DistributedDB::CipherType::AES_256_GCM;
        }
        option.observer = observer_;
        std::string fileName = meta.dataDir;
        ZLOGI("path=%{public}s storeId=%{public}s", fileName.c_str(), meta.GetStoreAlias().c_str());
        auto status = manager_->OpenStore(fileName, meta.storeId, option, delegate_);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("open store failed, path=%{public}s storeId=%{public}s status=%{public}d",
                fileName.c_str(), meta.GetStoreAlias().c_str(), status);
            return RDB_ERROR;
        }
        ZLOGI("open store success");
    }

    return RDB_OK;
}

std::pair<int32_t, int32_t> RdbSyncer::GetInstIndexAndUser(uint32_t tokenId, const std::string &bundleName)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return { 0, 0 };
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x appId:%{public}s", errCode, tokenId,
            bundleName.c_str());
        return { -1, -1 };
    }
    return { tokenInfo.instIndex, tokenInfo.userID };
}

DistributedDB::RelationalStoreDelegate* RdbSyncer::GetDelegate()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return delegate_;
}

int32_t RdbSyncer::SetDistributedTables(const std::vector<std::string> &tables, int32_t type)
{
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    for (const auto& table : tables) {
        ZLOGI("%{public}s", table.c_str());
        auto dBStatus = delegate->CreateDistributedTable(table, static_cast<DistributedDB::TableSyncType>(type));
        if (dBStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("create distributed table failed, table:%{public}s, err:%{public}d", table.c_str(), dBStatus);
            return RDB_ERROR;
        }
    }
    ZLOGI("create distributed table success");
    return RDB_OK;
}

std::vector<std::string> RdbSyncer::GetConnectDevices()
{
    auto deviceInfos = DmAdapter::GetInstance().GetRemoteDevices();
    std::vector<std::string> devices;
    for (const auto& deviceInfo : deviceInfos) {
        devices.push_back(deviceInfo.networkId);
    }
    ZLOGI("size=%{public}u", static_cast<uint32_t>(devices.size()));
    for (const auto& device: devices) {
        ZLOGI("%{public}s", Anonymous::Change(device).c_str());
    }
    return devices;
}

std::vector<std::string> RdbSyncer::NetworkIdToUUID(const std::vector<std::string> &networkIds)
{
    std::vector<std::string> uuids;
    for (const auto& networkId : networkIds) {
        auto uuid = DmAdapter::GetInstance().GetUuidByNetworkId(networkId);
        if (uuid.empty()) {
            ZLOGE("%{public}s failed", Anonymous::Change(networkId).c_str());
            continue;
        }
        uuids.push_back(uuid);
        ZLOGI("%{public}s <--> %{public}s", Anonymous::Change(networkId).c_str(), Anonymous::Change(uuid).c_str());
    }
    return uuids;
}

Details RdbSyncer::HandleSyncStatus(const std::map<std::string, std::vector<DistributedDB::TableStatus>> &syncStatus)
{
    Details details;
    for (const auto& status : syncStatus) {
        auto res = DistributedDB::DBStatus::OK;
        for (const auto& tableStatus : status.second) {
            if (tableStatus.status != DistributedDB::DBStatus::OK) {
                res = tableStatus.status;
                break;
            }
        }
        std::string uuid = DmAdapter::GetInstance().ToNetworkID(status.first);
        if (uuid.empty()) {
            ZLOGE("%{public}.6s failed", status.first.c_str());
            continue;
        }
        ZLOGI("%{public}.6s=%{public}d", uuid.c_str(), res);
        details[uuid].progress = SYNC_FINISH;
        details[uuid].code = int32_t(res);
    }
    return details;
}

Details RdbSyncer::HandleGenDetails(const GenDetails &details)
{
    Details dbDetails;
    for (const auto& [id, detail] : details) {
        auto &dbDetail = dbDetails[id];
        dbDetail.progress = detail.progress;
        dbDetail.code = detail.code;
        for (auto &[name, table] : detail.details) {
            auto &dbTable = dbDetail.details[name];
            Constant::Copy(&dbTable, &table);
        }
    }
    return dbDetails;
}

void RdbSyncer::EqualTo(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.EqualTo(operation.field_, operation.values_[0]);
    ZLOGI("field=%{public}s value=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::NotEqualTo(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.NotEqualTo(operation.field_, operation.values_[0]);
    ZLOGI("field=%{public}s value=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::And(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.And();
    ZLOGI("");
}

void RdbSyncer::Or(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.Or();
    ZLOGI("");
}

void RdbSyncer::OrderBy(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    bool isAsc = operation.values_[0] == "true";
    query.OrderBy(operation.field_, isAsc);
    ZLOGI("field=%{public}s isAsc=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::Limit(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    char *end = nullptr;
    int limit = static_cast<int>(strtol(operation.field_.c_str(), &end, DECIMAL_BASE));
    int offset = static_cast<int>(strtol(operation.values_[0].c_str(), &end, DECIMAL_BASE));
    if (limit < 0) {
        limit = 0;
    }
    if (offset < 0) {
        offset = 0;
    }
    query.Limit(limit, offset);
    ZLOGI("limit=%{public}d offset=%{public}d", limit, offset);
}

DistributedDB::Query RdbSyncer::MakeQuery(const PredicatesMemo &predicates)
{
    ZLOGI("table=%{public}zu", predicates.tables_.size());
    auto query = predicates.tables_.size() == 1 ? DistributedDB::Query::Select(*predicates.tables_.begin())
                                                : DistributedDB::Query::Select();
    if (predicates.tables_.size() > 1) {
        query.FromTable(predicates.tables_);
    }
    for (const auto& operation : predicates.operations_) {
        if (operation.operator_ >= 0 && operation.operator_ < OPERATOR_MAX) {
            HANDLES[operation.operator_](operation, query);
        }
    }
    return query;
}

int32_t RdbSyncer::DoSync(const Option &option, const PredicatesMemo &predicates, const AsyncDetail &async)
{
    ZLOGI("enter");
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    if (option.mode < DistributedData::GeneralStore::NEARBY_END) {
        auto &networkIds = predicates.devices_;
        auto devices = networkIds.empty() ? NetworkIdToUUID(GetConnectDevices()) : NetworkIdToUUID(networkIds);
        return delegate->Sync(
            devices, static_cast<DistributedDB::SyncMode>(option.mode), MakeQuery(predicates),
            [async](const std::map<std::string, std::vector<DistributedDB::TableStatus>> &syncStatus) {
                async(HandleSyncStatus(syncStatus));
            },
            !option.isAsync);
    } else if (option.mode < DistributedData::GeneralStore::CLOUD_END &&
               option.mode >= DistributedData::GeneralStore::CLOUD_BEGIN) {
        CloudEvent::StoreInfo storeInfo;
        storeInfo.bundleName = GetBundleName();
        storeInfo.user = AccountDelegate::GetInstance()->GetUserByToken(token_);
        storeInfo.storeName = GetStoreId();
        storeInfo.tokenId = token_;
        std::shared_ptr<RdbQuery> query = nullptr;
        if (!predicates.tables_.empty()) {
            query = std::make_shared<RdbQuery>();
            query->query_.FromTable(predicates.tables_);
        }

        auto info = ChangeEvent::EventInfo(option.mode, (option.isAsync ? 0 : WAIT_TIME), query,
            [async](const GenDetails &details) {
                async(HandleGenDetails(details));
            });
        auto evt = std::make_unique<ChangeEvent>(std::move(storeInfo), std::move(info));
        EventCenter::GetInstance().PostEvent(std::move(evt));
    }
    ZLOGI("delegate sync");
    return RDB_OK;
}

int32_t RdbSyncer::RemoteQuery(const std::string& device, const std::string& sql,
                               const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet)
{
    ZLOGI("enter");
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    ZLOGI("delegate remote query");
    std::shared_ptr<DistributedDB::ResultSet> dbResultSet;
    DistributedDB::DBStatus status = delegate->RemoteQuery(device, {sql, selectionArgs},
                                                           REMOTE_QUERY_TIME_OUT, dbResultSet);
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("DistributedDB remote query failed, status is  %{public}d.", status);
        return RDB_ERROR;
    }
    resultSet = new (std::nothrow) RdbResultSetImpl(dbResultSet);
    if (resultSet == nullptr) {
        ZLOGE("resultSet is nullptr");
        return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbSyncer::RemoveDeviceData()
{
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }
    DistributedDB::DBStatus status = delegate->RemoveDeviceData();
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("DistributedDB RemoveDeviceData failed, status is %{public}d.", status);
        return RDB_ERROR;
    }
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb

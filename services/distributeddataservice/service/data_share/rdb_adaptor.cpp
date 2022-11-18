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
#define LOG_TAG "RdbAdaptor"
#include "rdb_adaptor.h"

#include "log_print.h"
#include "permission_proxy.h"
#include "rdb_utils.h"
namespace OHOS::DataShare {
std::unique_ptr<Utils::Timer> RdbAdaptor::timer_ = nullptr;
ConcurrentMap<UriInfo, std::shared_ptr<RdbDelegate>> RdbAdaptor::delegates_;
uint32_t RdbAdaptor::timerId_ = 0;
int32_t RdbAdaptor::Insert(const UriInfo &uriInfo, const DataShareValuesBucket &valuesBucket, const int32_t userId)
{
    auto delegate = GetDelegate(uriInfo, userId);
    if (delegate == nullptr) {
        ZLOGE("delegate null");
        return -1;
    }
    return delegate->Insert(uriInfo.tableName, valuesBucket);
}
int32_t RdbAdaptor::Update(const UriInfo &uriInfo, const DataSharePredicates &predicate,
    const DataShareValuesBucket &valuesBucket, const int32_t userId)
{
    auto delegate = GetDelegate(uriInfo, userId);
    if (delegate == nullptr) {
        ZLOGE("delegate null");
        return -1;
    }
    return delegate->Update(uriInfo.tableName, predicate, valuesBucket);
}
int32_t RdbAdaptor::Delete(const UriInfo &uriInfo, const DataSharePredicates &predicate, const int32_t userId)
{
    auto delegate = GetDelegate(uriInfo, userId);
    if (delegate == nullptr) {
        ZLOGE("delegate null");
        return -1;
    }
    return delegate->Delete(uriInfo.tableName, predicate);
}
std::shared_ptr<DataShareResultSet> RdbAdaptor::Query(const UriInfo &uriInfo, const DataSharePredicates &predicates,
    const std::vector<std::string> &columns, const int32_t userId)
{
    auto delegate = GetDelegate(uriInfo, userId);
    if (delegate == nullptr) {
        ZLOGE("delegate null");
        return nullptr;
    }
    return delegate->Query(uriInfo.tableName, predicates, columns);
}

std::shared_ptr<RdbDelegate> RdbAdaptor::GetDelegate(const UriInfo &uriInfo, const int32_t userId)
{
    std::shared_ptr<RdbDelegate> value = nullptr;
    delegates_.Compute(uriInfo,
        [&uriInfo, &userId, &value](const UriInfo &key, std::shared_ptr<RdbDelegate> &delegate) {
        if (delegate != nullptr) {
            ZLOGD("has opened, reuse");
            value = delegate;
            return true;
        }
        DistributedData::StoreMetaData metaData;
        if (!PermissionProxy::QueryMetaData(
            uriInfo.bundleName, uriInfo.moduleName, uriInfo.storeName, metaData, userId)) {
            return false;
        }
        if (timer_ == nullptr) {
            timer_ = std::make_unique<Utils::Timer>("DataShareSlience");
            timer_->Setup();
        }
        timer_->Unregister(timerId_);
        timerId_ = timer_->Register(AutoClose, OPEN_TIME, true);

        delegate = std::make_shared<RdbDelegate>(metaData);
        value = delegate;
        return true;
    });

    return value;
}

void RdbAdaptor::AutoClose()
{
    ZLOGD("clear");
    delegates_.Clear();
}

RdbDelegate::RdbDelegate(const StoreMetaData &meta)
{
    int errCode = E_OK;
    RdbStoreConfig config(meta.dataDir);
    config.SetCreateNecessary(false);
    DefaultOpenCallback callback;
    store_ = RdbHelper::GetRdbStore(config, meta.version, callback, errCode);
    if (errCode != E_OK) {
        ZLOGE("GetRdbStore failed %{public}d, %{public}s", errCode, meta.storeId.c_str());
    }
}

RdbDelegate::~RdbDelegate()
{
    ZLOGI("destroy");
}

int64_t RdbDelegate::Insert(const std::string &tableName, const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int64_t rowId = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    int ret = store_->Insert(rowId, tableName, bucket);
    if (ret != E_OK) {
        ZLOGE("Insert failed %{public}d", ret);
    }
    return rowId;
}
int64_t RdbDelegate::Update(const std::string &tableName, const DataSharePredicates &predicate,
    const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int rowId = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Update(rowId, bucket, predicates);
    if (ret != E_OK) {
        ZLOGE("Insert failed %{public}d", ret);
    }
    return rowId;
}
int64_t RdbDelegate::Delete(const std::string &tableName, const DataSharePredicates &predicate)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int rowId = 0;
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Delete(rowId, predicates);
    if (ret != E_OK) {
        ZLOGE("Insert failed %{public}d", ret);
    }
    return rowId;
}
std::shared_ptr<DataShareResultSet> RdbDelegate::Query(const std::string &tableName,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(rdbPredicates, columns);
    if (resultSet == nullptr) {
        ZLOGE("Query failed");
        return nullptr;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return std::make_shared<DataShare::DataShareResultSet>(bridge);
}
} // namespace OHOS::DataShare
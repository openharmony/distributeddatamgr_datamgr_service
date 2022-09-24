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
int32_t RdbAdaptor::Insert(const std::string &bundleName, const std::string &moduleName, const std::string &storeName,
    const std::string &tableName, const DataShareValuesBucket &valuesBucket)
{
    DistributedData::StoreMetaData metaData;
    if (!PermissionProxy::QueryMetaData(bundleName, moduleName, storeName, metaData)) {
        return -1;
    }
    RdbDelegate delegate(metaData);
    return delegate.Insert(tableName, valuesBucket);
}
int32_t RdbAdaptor::Update(const std::string &bundleName, const std::string &moduleName, const std::string &storeName,
    const std::string &tableName, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    DistributedData::StoreMetaData metaData;
    if (!PermissionProxy::QueryMetaData(bundleName, moduleName, storeName, metaData)) {
        return -1;
    }
    RdbDelegate delegate(metaData);
    return delegate.Update(tableName, predicate, valuesBucket);
}
int32_t RdbAdaptor::Delete(const std::string &bundleName, const std::string &moduleName, const std::string &storeName,
    const std::string &tableName, const DataSharePredicates &predicate)
{
    DistributedData::StoreMetaData metaData;
    if (!PermissionProxy::QueryMetaData(bundleName, moduleName, storeName, metaData)) {
        return -1;
    }
    RdbDelegate delegate(metaData);
    return delegate.Delete(tableName, predicate);
}
std::shared_ptr<DataShareResultSet> RdbAdaptor::Query(const std::string &bundleName, const std::string &moduleName,
    const std::string &storeName, const std::string &tableName, const DataSharePredicates &predicates,
    const std::vector<std::string> &columns)
{
    DistributedData::StoreMetaData metaData;
    if (!PermissionProxy::QueryMetaData(bundleName, moduleName, storeName, metaData)) {
        return nullptr;
    }
    RdbDelegate delegate(metaData);
    return delegate.Query(tableName, predicates, columns);
}

RdbDelegate::RdbDelegate(const StoreMetaData &meta)
{
    int errCode = E_OK;
    RdbStoreConfig config(meta.dataDir);

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
int64_t RdbDelegate::Update(
    const std::string &tableName, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
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
std::shared_ptr<DataShareResultSet> RdbDelegate::Query(
    const std::string &tableName, const DataSharePredicates &predicates, const std::vector<std::string> &columns)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName);
    std::unique_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(rdbPredicates, columns);
    if (resultSet == nullptr) {
        ZLOGE("Query failed");
        return nullptr;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(std::move(resultSet));
    return std::make_shared<DataShare::DataShareResultSet>(bridge);
}
} // namespace OHOS::DataShare
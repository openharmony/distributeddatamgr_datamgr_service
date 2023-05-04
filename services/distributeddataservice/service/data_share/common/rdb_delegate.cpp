/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "rdb_delegate.h"

#include "log_print.h"
#include "rdb_utils.h"
#include "scheduler_manager.h"
#include "utils/anonymous.h"
namespace OHOS::DataShare {
constexpr static int32_t MAX_RESULTSET_COUNT = 16;
std::atomic<int32_t> RdbDelegate::resultSetCount = 0;
RdbDelegate::RdbDelegate(const std::string &dir, int version, int &errCode, bool registerFunction)
{
    RdbStoreConfig config(dir);
    config.SetCreateNecessary(false);
    DefaultOpenCallback callback;
    store_ = RdbHelper::GetRdbStore(config, version, callback, errCode);
    if (errCode != E_OK) {
        ZLOGE("GetRdbStore failed, errCode is %{public}d, dir is %{public}s", errCode,
            DistributedData::Anonymous::Change(dir).c_str());
    }
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
        ZLOGE("Insert failed %{public}s %{public}d", tableName.c_str(), ret);
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
    int changeCount = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Update(changeCount, bucket, predicates);
    if (ret != E_OK) {
        ZLOGE("Update failed  %{public}s %{public}d", tableName.c_str(), ret);
    }
    return changeCount;
}
int64_t RdbDelegate::Delete(const std::string &tableName, const DataSharePredicates &predicate)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int changeCount = 0;
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Delete(changeCount, predicates);
    if (ret != E_OK) {
        ZLOGE("Delete failed  %{public}s %{public}d", tableName.c_str(), ret);
    }
    return changeCount;
}
std::shared_ptr<DataShareResultSet> RdbDelegate::Query(
    const std::string &tableName, const DataSharePredicates &predicates,
    const std::vector<std::string> &columns, int &errCode)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    int count = resultSetCount.fetch_add(1);
    ZLOGI("start query %{public}d", count);
    if (count > MAX_RESULTSET_COUNT) {
        ZLOGE("resultSetCount is full");
        resultSetCount--;
        return nullptr;
    }
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(rdbPredicates, columns);
    if (resultSet == nullptr) {
        ZLOGE("Query failed %{public}s", tableName.c_str());
        resultSetCount--;
        return nullptr;
    }
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return std::shared_ptr<DataShareResultSet>(new DataShareResultSet(bridge), [](auto p) {
        ZLOGI("release resultset");
        resultSetCount--;
        delete p;
    });
}
class ResultSetJsonFormatter final: public DistributedData::Serializable {
public:
    explicit ResultSetJsonFormatter(const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
        : resultSet(resultSet)
    {
    }
    ~ResultSetJsonFormatter() {}
    bool Marshal(json &node) const override
    {
        int columnCount = 0;
        auto result = resultSet->GetColumnCount(columnCount);
        if (result != EOK) {
            ZLOGE("GetColumnCount err, %{public}d", result);
            return false;
        }
        while (resultSet->GoToNextRow() == E_OK) {
            json result;
            for (int i = 0; i < columnCount; i++) {
                std::string columnName;
                std::string value;
                resultSet->GetColumnName(i, columnName);
                resultSet->GetString(i, value);
                SetValue(result[columnName], value);
            }
            node.push_back(result);
        }
        return true;
    }
    bool Unmarshal(const json &node) override
    {
        return false;
    }

private:
    std::shared_ptr<NativeRdb::ResultSet> resultSet;
};

std::shared_ptr<DistributedData::Serializable> RdbDelegate::Query(
    const std::string &sql, const std::vector<std::string> &selectionArgs)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    std::shared_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(sql, selectionArgs);
    if (resultSet == nullptr) {
        ZLOGE("Query failed %{private}s", sql.c_str());
        return nullptr;
    }

    return std::make_shared<ResultSetJsonFormatter>(resultSet);
}

int RdbDelegate::ExecuteSql(const std::string &sql)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return -1;
    }
    return store_->ExecuteSql(sql);
}
} // namespace OHOS::DataShare
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

#define LOG_TAG "KvStorePredicates"

#include "cov_util.h"
#include "log_print.h"
#include "datashare_errno.h"
#include "kvstore_predicates.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
constexpr KvStorePredicates::QueryHandler KvStorePredicates::HANDLERS[LAST_TYPE];

Status KvStorePredicates::ToQuery(const DataSharePredicates &predicates, DataQuery &query)
{
    std::list<OperationItem> operationList = predicates.GetOperationList();  
    for (const auto &oper : operationList) {
        if (oper.operation < 0 || oper.operation >= LAST_TYPE) {
            ZLOGE("operation param error");
            return Status::NOT_SUPPORT;
        }
        Status status = (this->*HANDLERS[oper.operation])(oper, query);
        if (status != Status::SUCCESS) {
            ZLOGE("ToQuery called failed: %{public}d", status);
            return status;
        }
    }
    return Status::SUCCESS;
}

Status KvStorePredicates::GetKeys(const DataSharePredicates &predicates, std::vector<Key> &keys)
{
    std::list<OperationItem> operationList = predicates.GetOperationList();
    if (operationList.empty()) {
        ZLOGE("operationList is null");
        return Status::ERROR;
    }

    std::vector<std::string> myKeys;
    for (const auto &oper : operationList) {
        if (oper.operation != IN_KEY) {
            ZLOGE("find operation failed");
            return Status::NOT_SUPPORT;
        }
        std::vector<std::string> val;
        int status = oper.para1.GetStringVector(val);
        if (status != E_OK) {
            ZLOGE("GetStringVector failed: %{public}d", status);
            return Status::ERROR;
        }
        myKeys.insert(myKeys.end(), val.begin(), val.end());
    }
    for (const auto &it : myKeys) {
        keys.push_back(it.c_str());
    }
    return Status::SUCCESS;
}

Status KvStorePredicates::InKeys(const OperationItem &oper, DataQuery &query)
{
    std::vector<std::string> keys;
    int status = oper.para1.GetStringVector(keys);
    if (status != E_OK) {
        ZLOGE("GetStringVector failed: %{public}d", status);
        return Status::ERROR;
    }
    query.InKeys(keys);
    return Status::SUCCESS;
}

Status KvStorePredicates::KeyPrefix(const OperationItem &oper, DataQuery &query)
{
    std::string prefix;
    int status = oper.para1.GetString(prefix);
    if (status != E_OK) {
        ZLOGE("KeyPrefix failed: %{public}d", status);
        return Status::ERROR;
    }
    query.KeyPrefix(prefix);
    return Status::SUCCESS;
}

Status KvStorePredicates::EqualTo(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys equal(&query, QueryType::EQUAL);
    CovUtil::FillField(field, oper.para2.value, equal);
    return Status::SUCCESS;
}

Status KvStorePredicates::NotEqualTo(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys notEqual(&query, QueryType::NOT_EQUAL);
    CovUtil::FillField(field, oper.para2.value, notEqual);
    return Status::SUCCESS;
}

Status KvStorePredicates::GreaterThan(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys greater(&query, QueryType::GREATER);
    CovUtil::FillField(field, oper.para2.value, greater);
    return Status::SUCCESS;
}

Status KvStorePredicates::LessThan(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys less(&query, QueryType::LESS);
    CovUtil::FillField(field, oper.para2.value, less);
    return Status::SUCCESS;
}

Status KvStorePredicates::GreaterThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys greaterOrEqual(&query, QueryType::GREATER_OR_EQUAL);
    CovUtil::FillField(field, oper.para2.value, greaterOrEqual);
    return Status::SUCCESS;
}

Status KvStorePredicates::LessThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    Querys lessOrEqual(&query, QueryType::LESS_OR_EQUAL);
    CovUtil::FillField(field, oper.para2.value, lessOrEqual);
    return Status::SUCCESS;
}

Status KvStorePredicates::And(const OperationItem &oper, DataQuery &query)
{
    query.And();
    return Status::SUCCESS;
}

Status KvStorePredicates::Or(const OperationItem &oper, DataQuery &query)
{
    query.Or();
    return Status::SUCCESS;
}

Status KvStorePredicates::IsNull(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("IsNull failed: %{public}d", status);
        return Status::ERROR;
    }
    query.IsNull(field);
    return Status::SUCCESS;
}

Status KvStorePredicates::IsNotNull(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("IsNotNull failed: %{public}d", status);
        return Status::ERROR;
    }
    query.IsNotNull(field);
    return Status::SUCCESS;
}

Status KvStorePredicates::In(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    DistributedKv::In in(&query);
    CovUtil::FillField(field, oper.para2.value, in);
    return Status::SUCCESS;
}

Status KvStorePredicates::NotIn(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("GetString failed: %{public}d", status);
        return Status::ERROR;
    }
    DistributedKv::NotIn notIn(&query);
    CovUtil::FillField(field, oper.para2.value, notIn);
    return Status::SUCCESS;
}

Status KvStorePredicates::Like(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("Like field failed: %{public}d", status);
        return Status::ERROR;
    }
    
    std::string value;
    status = oper.para2.GetString(value);
    if (status != E_OK) {
        ZLOGE("Like value failed: %{public}d", status);
        return Status::ERROR;
    }
    query.Like(field, value);
    return Status::SUCCESS;
}

Status KvStorePredicates::Unlike(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("Unlike failed: %{public}d", status);
        return Status::ERROR;
    }
    
    std::string value;
    status = oper.para2.GetString(value);
    if (status != E_OK) {
        ZLOGE("Unlike value failed: %{public}d", status);
        return Status::ERROR;
    }
    query.Unlike(field, value);
    return Status::SUCCESS;
}

Status KvStorePredicates::OrderByAsc(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("OrderByAsc failed: %{public}d", status);
        return Status::ERROR;
    }
    query.OrderByAsc(field);
    return Status::SUCCESS;
}

Status KvStorePredicates::OrderByDesc(const OperationItem &oper, DataQuery &query)
{
    std::string field;
    int status = oper.para1.GetString(field);
    if (status != E_OK) {
        ZLOGE("OrderByDesc failed: %{public}d", status);
        return Status::ERROR;
    }
    query.OrderByDesc(field);
    return Status::SUCCESS;
}

Status KvStorePredicates::Limit(const OperationItem &oper, DataQuery &query)
{
    int number;
    int status = oper.para1.GetInt(number);
    if (status != E_OK) {
        ZLOGE("Limit  number failed: %{public}d", status);
        return Status::ERROR;
    }
    
    int offset;
    status = oper.para2.GetInt(offset);
    if (status != E_OK) {
        ZLOGE("Limit offset failed: %{public}d", offset);
        return Status::ERROR;
    }
    query.Limit(number, offset);
    return Status::SUCCESS;
}
} // namespace DistributedKv
} // namespace OHOS
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

#include "kvstore_predicates.h"
#include "log_print.h"
#include "datashare_errno.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
constexpr KvStorePredicates::QueryHandler KvStorePredicates::HANDLERS[LAST_TYPE];

Status KvStorePredicates::GetContext(const OperationItem &oper, Context &context)
{
    int status = oper.para1.GetString(context.field);
    if (status != E_OK) {
        ZLOGE("Get context.field failed: %{public}d", status);
        return Status::ERROR;
        }
        
    auto innerType = oper.para2.GetType();
    switch (innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            status = oper.para2.GetInt(context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            status = oper.para2.GetLong(context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            status = oper.para2.GetDouble(context.doubleValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_BOOL: {
            status = oper.para2.GetBool(context.boolValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            status = oper.para2.GetString(context.stringValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_INT_VECTOR: {
            status = oper.para2.GetIntVector(context.intList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG_VECTOR: {
            status = oper.para2.GetLongVector(context.longList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE_VECTOR: {
            status = oper.para2.GetDoubleVector(context.doubleList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_STRING_VECTOR: {
            status = oper.para2.GetStringVector(context.stringList);
            break;
        }
        default:
            status = Status::ERROR;
            break;
    }

    if(status != Status::SUCCESS) {
        ZLOGE("GetType failed: %{public}d", status);
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

Status KvStorePredicates::ToQuery(const DataSharePredicates &predicates, DataQuery &query)
{
    std::list<OperationItem> operationList = predicates.GetOperationList();
    if (operationList.empty()) {
        ZLOGE("ToQuery operationList is null");
        return Status::INVALID_ARGUMENT;
    }
    
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
    for(const auto &oper : operationList)
    { 
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
    for (const auto &it : myKeys)
    {
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
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("EqualTo GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.EqualTo(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.EqualTo(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.EqualTo(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_BOOL: {
            query.EqualTo(context.field, context.boolValue);
            break;        
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.EqualTo(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("EqualTo failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::NotEqualTo(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("NotEqualTo GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.NotEqualTo(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.NotEqualTo(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.NotEqualTo(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_BOOL: {
            query.NotEqualTo(context.field, context.boolValue);
            break;        
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.NotEqualTo(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("NotEqualTo failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::GreaterThan(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("GreaterThan GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.GreaterThan(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.GreaterThan(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.GreaterThan(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.GreaterThan(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("GreaterThan failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::LessThan(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("LessThan GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.LessThan(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.LessThan(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.LessThan(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.LessThan(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("LessThan failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::GreaterThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("GreaterThanOrEqualTo GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.GreaterThanOrEqualTo(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.GreaterThanOrEqualTo(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.GreaterThanOrEqualTo(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.GreaterThanOrEqualTo(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("GreaterThanOrEqualTo failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::LessThanOrEqualTo(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("LessThanOrEqualTo GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT: {
            query.LessThanOrEqualTo(context.field, context.intValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG: {
            query.LessThanOrEqualTo(context.field, context.longValue);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE: {
            query.LessThanOrEqualTo(context.field, context.doubleValue);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING: {
            query.LessThanOrEqualTo(context.field, context.stringValue);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("LessThanOrEqualTo failed: %{public}d", status);
            break;
    }
    return status;
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
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("In GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT_VECTOR: {
            query.In(context.field, context.intList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG_VECTOR: {
            query.In(context.field, context.longList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE_VECTOR: {
            query.In(context.field, context.doubleList);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING_VECTOR: {
            query.In(context.field, context.stringList);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("In failed: %{public}d", status);
            break;
    }
    return status;
}

Status KvStorePredicates::NotIn(const OperationItem &oper, DataQuery &query)
{
    Context context;
    Status status = GetContext(oper, context);
    if (status != Status::SUCCESS) {
        ZLOGE("NotIn GetContext called failed: %{public}d", status);
        return status;
    }
    
    switch (context.innerType) {
        case DataSharePredicatesObjectType::TYPE_INT_VECTOR: {
            query.NotIn(context.field, context.intList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_LONG_VECTOR: {
            query.NotIn(context.field, context.longList);
            break;
        }
        case DataSharePredicatesObjectType::TYPE_DOUBLE_VECTOR: {
            query.NotIn(context.field, context.doubleList);
            break;  
        }
        case DataSharePredicatesObjectType::TYPE_STRING_VECTOR: {
            query.NotIn(context.field, context.stringList);
            break;             
        }
        default: 
            status = Status::ERROR;
            ZLOGE("NotIn failed: %{public}d", status);
            break;
    }
    return status;
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
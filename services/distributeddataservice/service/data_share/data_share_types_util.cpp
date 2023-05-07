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
#define LOG_TAG "DataShareTypesUtil"
#include "data_share_types_util.h"

#include "log_print.h"
namespace OHOS::ITypesUtil {
using namespace OHOS::DataShare;
template<>
bool Unmarshalling(Predicates &predicates, MessageParcel &parcel)
{
    ZLOGD("Unmarshalling DataSharePredicates Start");
    std::vector<Operation> operations {};
    std::string whereClause = "";
    std::vector<std::string> whereArgs;
    std::string order = "";
    int16_t mode = DataShare::INVALID_MODE;
    if (!ITypesUtil::Unmarshal(parcel, operations, whereClause, whereArgs, order, mode)) {
        ZLOGE("predicate read whereClause failed");
        return false;
    }
    DataShare::DataSharePredicates tmpPredicates(operations);
    tmpPredicates.SetWhereClause(whereClause);
    tmpPredicates.SetWhereArgs(whereArgs);
    tmpPredicates.SetOrder(order);
    tmpPredicates.SetSettingMode(mode);
    predicates = tmpPredicates;
    return true;
}

template<>
bool Unmarshalling(Operation &operation, MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel, operation.operation, operation.singleParams, operation.multiParams);
}

template<>
bool Unmarshalling(PublishedDataItem &dataItem, MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel, dataItem.key_, dataItem.subscriberId_, dataItem.value_);
}

template<>
bool Marshalling(const PublishedDataItem &dataItem, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, dataItem.key_, dataItem.subscriberId_, dataItem.value_);
}

template<>
bool Unmarshalling(Data &data, MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel, data.datas_, data.version_);
}

template<>
bool Unmarshalling(TemplateId &templateId, MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel, templateId.subscriberId_, templateId.bundleName_);
}

template<>
bool Marshalling(const TemplateId &templateId, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, templateId.subscriberId_, templateId.bundleName_);
}

template<>
bool Unmarshalling(PredicateTemplateNode &predicateTemplateNode, MessageParcel &parcel)
{
    return ITypesUtil::Unmarshal(parcel, predicateTemplateNode.key_, predicateTemplateNode.selectSql_);
}

template<>
bool Marshalling(const RdbChangeNode &changeNode, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, changeNode.uri_, changeNode.templateId_, changeNode.data_);
}

template<>
bool Marshalling(const PublishedDataChangeNode &changeNode, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, changeNode.ownerBundleName_, changeNode.datas_);
}

template<>
bool Marshalling(const OperationResult &operationResult, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, operationResult.key_, operationResult.errCode_);
}

template<>
bool Unmarshalling(AshmemNode &node, MessageParcel &parcel)
{
    node.isManaged = true;
    node.ashmem = parcel.ReadAshmem();
    return true;
}

template<>
bool Marshalling(const AshmemNode &node, MessageParcel &parcel)
{
    return parcel.WriteAshmem(node.ashmem);
}
}
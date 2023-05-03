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
    dataItem.key_ = parcel.ReadString();
    dataItem.subscriberId_ = parcel.ReadInt64();
    auto index = parcel.ReadUint32();
    ZLOGE("hanlu Unmarshalling %{public}s",  dataItem.key_.c_str());
    if (index == 0) {
        sptr<Ashmem> ashmem = parcel.ReadAshmem();
        dataItem.value_ = ashmem;
        ZLOGE("hanlu ReadAshmem %{public}p", ashmem.GetRefPtr());
        bool ret = ashmem->MapReadOnlyAshmem();
        if (!ret) {
            ZLOGE("MapReadAndWriteAshmem fail, %{private}s", dataItem.key_.c_str());
            return false;
        }
        ZLOGE("hanlu MapReadAndWriteAshmem %{public}p", ashmem.GetRefPtr());
    } else {
        dataItem.value_ = parcel.ReadString();
    }
    return true;
}

template<>
bool Marshalling(const PublishedDataItem &dataItem, MessageParcel &parcel)
{
    if (!parcel.WriteString(dataItem.key_)) {
        return false;
    }
    if (!parcel.WriteInt64(dataItem.subscriberId_)) {
        return false;
    }
    auto index = static_cast<uint32_t>(dataItem.value_.index());
    if (!parcel.WriteUint32(index)) {
        return false;
    }
    if (index == 0) {
        sptr<Ashmem> ashmem = std::get<sptr<Ashmem>>(dataItem.value_);
        if (ashmem == nullptr) {
            ZLOGE("ashmem null");
            return false;
        }
        return parcel.WriteAshmem(ashmem);
    }
    return parcel.WriteString(std::get<std::string>(dataItem.value_));
}

template<>
bool Unmarshalling(Data &data, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }
    size_t size = static_cast<size_t>(len);
    size_t readAbleSize = parcel.GetReadableBytes();
    if ((size > readAbleSize) || (size > data.datas_.max_size())) {
        return false;
    }
    std::vector<PublishedDataItem> dataItems;
    for (size_t i = 0; i < size; i++) {
        PublishedDataItem value;
        if (!Unmarshalling(value, parcel)) {
            return false;
        }
        dataItems.emplace_back(std::move(value));
    }
    data.datas_ = dataItems;
    data.version_ = parcel.ReadInt32();
    return true;
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
    if (!parcel.WriteString(changeNode.ownerBundleName_)) {
        return false;
    }
    if (!parcel.WriteInt32(changeNode.datas_.size())) {
        return false;
    }
    for (const auto &dataItem : changeNode.datas_) {
        if (!Marshalling(dataItem, parcel)) {
            return false;
        }
    }
    return true;
}

template<>
bool Marshalling(const OperationResult &operationResult, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, operationResult.key_, operationResult.errCode_);
}
}
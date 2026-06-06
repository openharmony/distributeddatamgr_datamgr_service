/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_TYPE_UTILS_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_TYPE_UTILS_H
#include <string>
#include <vector>

#include "big_integer.h"
#include "itypes_util.h"
#include "rdb_service.h"
#include "rdb_types.h"
#include "value_object.h"
#include "values_bucket.h"

namespace OHOS::ITypesUtil {
using SubOption = DistributedRdb::SubscribeOption;
using NotifyConfig = DistributedRdb::RdbNotifyConfig;
using Option = DistributedRdb::RdbService::Option;
using RdbChangedData = DistributedRdb::RdbChangedData;
using Reference = DistributedRdb::Reference;
using StatReporter = DistributedRdb::RdbStatEvent;
using RdbPredicates = DistributedRdb::PredicatesMemo;
using RdbOperation = DistributedRdb::RdbPredicateOperation;
using SyncerParam = DistributedRdb::RdbSyncerParam;
using Origin = DistributedRdb::Origin;
using BigInt = NativeRdb::BigInteger;
using RdbProperties = DistributedRdb::RdbChangeProperties;
using RdbDebugInfo = DistributedRdb::RdbDebugInfo;
using RdbDfxInfo = DistributedRdb::RdbDfxInfo;
using ProgressDetail = DistributedRdb::ProgressDetail;
using TableDetail = DistributedRdb::TableDetail;
using Statistic = DistributedRdb::Statistic;
using Observer = DistributedRdb::RdbStoreObserver;
using PrimaryKey = Observer::PrimaryKey;
using PrimaryKeys = std::vector<PrimaryKey>[Observer::CHG_TYPE_BUTT];
using ValueObject = NativeRdb::ValueObject;
using ValuesBucket = NativeRdb::ValuesBucket;
using Asset = NativeRdb::AssetValue;

template<>
bool Marshalling(const NotifyConfig &input, MessageParcel &data);

template<>
bool Marshalling(const Option &input, MessageParcel &data);

template<>
bool Marshalling(const SubOption &input, MessageParcel &data);

template<>
bool Marshalling(const RdbChangedData &input, MessageParcel &data);

template<>
bool Marshalling(const Reference &input, MessageParcel &data);

template<>
bool Marshalling(const StatReporter &input, MessageParcel &data);

template<>
bool Unmarshalling(NotifyConfig &output, MessageParcel &data);

template<>
bool Unmarshalling(Option &output, MessageParcel &data);

template<>
bool Unmarshalling(SubOption &output, MessageParcel &data);

template<>
bool Unmarshalling(RdbChangedData &output, MessageParcel &data);

template<>
bool Unmarshalling(Reference &output, MessageParcel &data);

template<>
bool Unmarshalling(StatReporter &output, MessageParcel &data);

template<>
bool Marshalling(const RdbPredicates &input, MessageParcel &data);

template<>
bool Unmarshalling(RdbPredicates &output, MessageParcel &data);

template<>
bool Marshalling(const RdbOperation &input, MessageParcel &data);

template<>
bool Unmarshalling(RdbOperation &output, MessageParcel &data);

template<>
bool Marshalling(const SyncerParam &input, MessageParcel &data);

template<>
bool Unmarshalling(SyncerParam &output, MessageParcel &data);

template<>
bool Marshalling(const Origin &input, MessageParcel &data);

template<>
bool Unmarshalling(Origin &output, MessageParcel &data);

template<>
bool Marshalling(const BigInt &input, MessageParcel &data);

template<>
bool Unmarshalling(BigInt &output, MessageParcel &data);

template<>
bool Marshalling(const RdbProperties &input, MessageParcel &data);

template<>
bool Unmarshalling(RdbProperties &output, MessageParcel &data);

template<>
bool Marshalling(const RdbDebugInfo &input, MessageParcel &data);

template<>
bool Unmarshalling(RdbDebugInfo &output, MessageParcel &data);

template<>
bool Marshalling(const RdbDfxInfo &input, MessageParcel &data);

template<>
bool Unmarshalling(RdbDfxInfo &output, MessageParcel &data);

template<>
bool Marshalling(const ProgressDetail &input, MessageParcel &data);

template<>
bool Unmarshalling(ProgressDetail &output, MessageParcel &data);

template<>
bool Marshalling(const TableDetail &input, MessageParcel &data);

template<>
bool Unmarshalling(TableDetail &output, MessageParcel &data);

template<>
bool Marshalling(const Statistic &input, MessageParcel &data);

template<>
bool Unmarshalling(Statistic &output, MessageParcel &data);

template<>
bool Marshalling(const PrimaryKeys &input, MessageParcel &data);

template<>
bool Marshalling(const ValueObject &input, MessageParcel &data);

template<>
bool Unmarshalling(ValueObject &output, MessageParcel &data);

template<>
bool Marshalling(const ValuesBucket &input, MessageParcel &data);

template<>
bool Unmarshalling(ValuesBucket &output, MessageParcel &data);

template<>
bool Marshalling(const Asset &input, MessageParcel &data);

template<>
bool Unmarshalling(Asset &output, MessageParcel &data);
} // namespace OHOS::ITypesUtil
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_TYPE_UTILS_H
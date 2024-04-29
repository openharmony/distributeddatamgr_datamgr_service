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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_TYPES_UTIL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_TYPES_UTIL_H
#include "cloud_types.h"
#include "itypes_util.h"
#include "cloud_types.h"
#include "values_bucket.h"

namespace OHOS::ITypesUtil {
using Participant = OHOS::CloudData::Participant;
using Privilege = OHOS::CloudData::Privilege;
using Role = OHOS::CloudData::Role;
using Confirmation = OHOS::CloudData::Confirmation;
using SharingCode = OHOS::CloudData::SharingCode;
using Asset = OHOS::NativeRdb::AssetValue;
using ValueObject = OHOS::NativeRdb::ValueObject;
using ValuesBucket = OHOS::NativeRdb::ValuesBucket;
using StatisticInfo = OHOS::CloudData::StatisticInfo;
using Strategy = OHOS::CloudData::Strategy;
using CloudSyncInfo = OHOS::CloudData::CloudSyncInfo;

template<>
bool Marshalling(const Participant &input, MessageParcel &data);
template<>
bool Unmarshalling(Participant &output, MessageParcel &data);

template<>
bool Marshalling(const Privilege &input, MessageParcel &data);
template<>
bool Unmarshalling(Privilege &output, MessageParcel &data);

template<>
bool Marshalling(const Asset &input, MessageParcel &data);
template<>
bool Unmarshalling(Asset &output, MessageParcel &data);
template<>
bool Marshalling(const ValueObject &input, MessageParcel &data);
template<>
bool Unmarshalling(ValueObject &output, MessageParcel &data);
template<>
bool Marshalling(const ValuesBucket &input, MessageParcel &data);
template<>
bool Unmarshalling(ValuesBucket &output, MessageParcel &data);

template<>
bool Marshalling(const StatisticInfo &input, MessageParcel &data);
template<>
bool Unmarshalling(StatisticInfo &output, MessageParcel &data);

template<>
bool Unmarshalling(Strategy &output, MessageParcel &data);

template<>
bool Marshalling(const CloudSyncInfo &input, MessageParcel &data);
template<>
bool Unmarshalling(CloudSyncInfo &output, MessageParcel &data);
} // namespace OHOS::ITypesUtil
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLOUD_CLOUD_TYPES_UTIL_H

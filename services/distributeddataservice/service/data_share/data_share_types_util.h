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

#ifndef DATASHARESERVICE_DATA_SHARE_TYPES_UTIL_H
#define DATASHARESERVICE_DATA_SHARE_TYPES_UTIL_H
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "itypes_util.h"
namespace OHOS::ITypesUtil {
using Predicates = DataShare::DataSharePredicates;
using Operation = DataShare::OperationItem;
template<>
bool Unmarshalling(Predicates &predicates, MessageParcel &parcel);

template<>
bool Unmarshalling(Operation &operation, MessageParcel &parcel);
};

#endif // DATASHARESERVICE_DATA_SHARE_TYPES_UTIL_H

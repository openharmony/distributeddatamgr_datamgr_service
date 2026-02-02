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

#include "rdb_types_utils.h"
namespace OHOS::ITypesUtil {
template<>
bool Unmarshalling(NotifyConfig &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.delay_, output.isFull_);
}
 
template<>
bool Unmarshalling(Option &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(
        data, output.mode, output.seqNum, output.isAsync, output.isAutoSync, output.isCompensation);
}
 
template<>
bool Unmarshalling(SubOption &output, MessageParcel &data)
{
    int32_t mode = static_cast<int32_t>(output.mode);
    auto ret = ITypesUtil::Unmarshal(data, mode);
    output.mode = static_cast<decltype(output.mode)>(mode);
    return ret;
}
 
template<>
bool Unmarshalling(RdbChangedData &output, MessageParcel &data)
{
    return Unmarshal(data, output.tableData);
}
 
template<>
bool Unmarshalling(RdbProperties &output, MessageParcel &data)
{
    return Unmarshal(data, output.isTrackedDataChange, output.isP2pSyncDataChange);
}
 
template<>
bool Unmarshalling(Reference &output, MessageParcel &data)
{
    return Unmarshal(data, output.sourceTable, output.targetTable, output.refFields);
}
 
template<>
bool Unmarshalling(StatReporter &output, MessageParcel &data)
{
    return Unmarshal(data, output.statType, output.bundleName, output.storeName, output.subType, output.costTime);
}
} // namespace OHOS::ITypesUtil
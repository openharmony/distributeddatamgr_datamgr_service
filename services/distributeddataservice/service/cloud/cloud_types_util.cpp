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

#include "cloud_types_util.h"

namespace OHOS::ITypesUtil {
template<>
bool Marshalling(const Participant &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(
        data, input.identity, input.role, input.state, input.privilege, input.attachInfo);
}

template<>
bool Unmarshalling(Participant &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(
        data, output.identity, output.role, output.state, output.privilege, output.attachInfo);
}

template<>
bool Marshalling(const Privilege &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.writable, input.readable,
        input.creatable, input.deletable, input.shareable);
}

template<>
bool Unmarshalling(Privilege &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.writable, output.readable,
        output.creatable, output.deletable, output.shareable);
}

template<>
bool Marshalling(const Asset &input, MessageParcel &data)
{
    return Marshal(data, input.version, input.name, input.size, input.modifyTime, input.uri);
}
template<>
bool Unmarshalling(Asset &output, MessageParcel &data)
{
    return Unmarshal(data, output.version, output.name, output.size, output.modifyTime, output.uri);
}
template<>
bool Marshalling(const ValueObject &input, MessageParcel &data)
{
    return Marshal(data, input.value);
}
template<>
bool Unmarshalling(ValueObject &output, MessageParcel &data)
{
    return Unmarshal(data, output.value);
}
template<>
bool Marshalling(const ValuesBucket &input, MessageParcel &data)
{
    return Marshal(data, input.values_);
}
template<>
bool Unmarshalling(ValuesBucket &output, MessageParcel &data)
{
    return Unmarshal(data, output.values_);
}

template<>
bool Marshalling(const StatisticInfo &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.table, input.inserted, input.updated, input.normal);
}

template<>
bool Unmarshalling(StatisticInfo &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.table, output.inserted, output.updated, output.normal);
}

template<>
bool Unmarshalling(Strategy &output, MessageParcel &data)
{
    uint32_t result;
    if (!data.ReadUint32(result) || result < Strategy::STRATEGY_HEAD || result >= Strategy::STRATEGY_BUTT) {
        return false;
    }
    output = static_cast<Strategy>(result);
    return true;
}

template<>
bool Marshalling(const CloudSyncInfo &input, MessageParcel &data)
{
    return Marshal(data, input.startTime, input.finishTime, input.code);
}

template<>
bool Unmarshalling(CloudSyncInfo &output, MessageParcel &data)
{
    return Unmarshal(data, output.startTime, output.finishTime, output.code);
}
} // namespace OHOS::ITypesUtil
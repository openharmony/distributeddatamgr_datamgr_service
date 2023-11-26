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
        data, input.identity, input.role, input.status, input.privilege, input.attachInfo);
}
template<>
bool Unmarshalling(Participant &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(
        data, output.identity, output.role, output.status, output.privilege, output.attachInfo);
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
bool Marshalling(const Role &input, MessageParcel &data)
{
    return data.WriteInt32(static_cast<int32_t>(input));
}

template<>
bool Unmarshalling(Role &output, MessageParcel &data)
{
    int32_t result;
    if (!data.ReadInt32(result) || result <=  Role::ROLE_NIL || result >=  Role::ROLE_BUTT) {
        return false;
    }
    output = static_cast<Role>(result);
    return true;
}

template<>
bool Marshalling(const Confirmation &input, MessageParcel &data)
{
    return data.WriteInt32(static_cast<int32_t>(input));
}

template<>
bool Unmarshalling(Confirmation &output, MessageParcel &data)
{
    int32_t result;
    if (!data.ReadInt32(result) || result <= Confirmation::CFM_NIL ||
        result >= Confirmation::CFM_BUTT) {
        return false;
    }
    output = static_cast<Confirmation>(result);
    return true;
}

template<>
bool Marshalling(const SharingCode &input, MessageParcel &data)
{
    return data.WriteInt32(static_cast<int32_t>(input));
}

template<>
bool Unmarshalling(SharingCode &output, MessageParcel &data)
{
    int32_t result;
    if (!data.ReadInt32(result) || result < SharingCode::SUCCESS) {
        return false;
    }
    output = static_cast< SharingCode>(result);
    return true;
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
} // namespace OHOS::ITypesUtil
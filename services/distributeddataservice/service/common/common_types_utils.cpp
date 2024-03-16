/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "common_types_utils.h"

namespace OHOS::ITypesUtil {
template<>
bool Marshalling(const Asset &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.name, input.uri, input.path, input.createTime,
        input.modifyTime, input.size, input.status, input.hash);
}
template<>
bool Unmarshalling(Asset &output, MessageParcel &data)
{
    return Unmarshal(data, output.name, output.uri, output.path, output.createTime,
        output.modifyTime, output.size, output.status, output.hash);
}
}
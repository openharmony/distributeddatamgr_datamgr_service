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

#include "metadata/matrix_meta_data.h"
#include "utils/constant.h"
namespace OHOS::DistributedData {
bool MatrixMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(mask)], mask);
    SetValue(node[GET_NAME(deviceId)], deviceId);
    SetValue(node[GET_NAME(maskInfo)], maskInfo);
    return true;
}

bool MatrixMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(mask), mask);
    GetValue(node, GET_NAME(deviceId), deviceId);
    GetValue(node, GET_NAME(maskInfo), maskInfo);
    return true;
}

std::string MatrixMetaData::GetKey() const
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, { deviceId });
}

std::string MatrixMetaData::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData
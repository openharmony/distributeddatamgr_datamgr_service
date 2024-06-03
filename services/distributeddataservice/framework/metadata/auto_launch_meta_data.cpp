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

#include "metadata/auto_launch_meta_data.h"

#include "utils/constant.h"
namespace OHOS::DistributedData {

AutoLaunchMetaData::AutoLaunchMetaData()
{
}

bool AutoLaunchMetaData::Marshal(json& node) const
{
    SetValue(node[GET_NAME(datas)], datas);
    return true;
}

bool AutoLaunchMetaData::Unmarshal(const json& node)
{
    GetValue(node, GET_NAME(datas), datas);
    return true;
}

std::string AutoLaunchMetaData::GetPrefix(const std::initializer_list<std::string>& fields)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, fields);
}
}
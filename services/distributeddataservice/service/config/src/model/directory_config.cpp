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

#include "model/directory_config.h"
namespace OHOS {
namespace DistributedData {
bool DirectoryConfig::DirectoryStrategy::Marshal(json &node) const
{
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(pattern)], pattern);
    SetValue(node[GET_NAME(metaPath)], metaPath);
    SetValue(node[GET_NAME(autoCreate)], autoCreate);
    return true;
}

bool DirectoryConfig::DirectoryStrategy::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(pattern), pattern);
    GetValue(node, GET_NAME(metaPath), metaPath);
    GetValue(node, GET_NAME(autoCreate), autoCreate);
    return true;
}

bool DirectoryConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(strategy)], strategy);
    return true;
}

bool DirectoryConfig::Unmarshal(const json &node)
{
    return GetValue(node, GET_NAME(strategy), strategy);
}
} // namespace DistributedData
} // namespace OHOS

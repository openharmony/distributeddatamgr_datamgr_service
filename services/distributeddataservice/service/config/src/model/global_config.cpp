/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "model/global_config.h"
namespace OHOS {
namespace DistributedData {
bool GlobalConfig::Marshal(json &node) const
{
    SetValue(node[GET_NAME(processLabel)], processLabel);
    SetValue(node[GET_NAME(metaData)], metaData);
    SetValue(node[GET_NAME(version)], version);
    SetValue(node[GET_NAME(features)], features);
    SetValue(node[GET_NAME(components)], components);
    SetValue(node[GET_NAME(bundleChecker)], bundleChecker);
    SetValue(node[GET_NAME(networks)], networks);
    SetValue(node[GET_NAME(directory)], directory);
    SetValue(node[GET_NAME(backup)], backup);
    return true;
}

bool GlobalConfig::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(processLabel), processLabel);
    GetValue(node, GET_NAME(metaData), metaData);
    GetValue(node, GET_NAME(version), version);
    GetValue(node, GET_NAME(features), features);
    GetValue(node, GET_NAME(components), components);
    GetValue(node, GET_NAME(bundleChecker), bundleChecker);
    GetValue(node, GET_NAME(networks), networks);
    GetValue(node, GET_NAME(directory), directory);
    GetValue(node, GET_NAME(backup), backup);
    return true;
}

GlobalConfig::~GlobalConfig()
{
    delete components;
    delete bundleChecker;
    delete networks;
    delete directory;
    delete backup;
}
} // namespace DistributedData
} // namespace OHOS

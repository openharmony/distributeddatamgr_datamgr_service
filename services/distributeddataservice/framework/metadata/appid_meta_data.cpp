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
#include "metadata/appid_meta_data.h"

namespace OHOS {
namespace DistributedData {
bool AppIDMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    return true;
}

bool AppIDMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(bundleName), bundleName);
    return true;
}

AppIDMetaData::AppIDMetaData()
{
}

AppIDMetaData::AppIDMetaData(const std::string &appId, const std::string &bundleName)
    : appId(appId), bundleName(bundleName)
{
}

AppIDMetaData::~AppIDMetaData()
{
}

bool AppIDMetaData::operator==(const AppIDMetaData &appIdMetaData) const
{
    return (appId == appIdMetaData.appId && bundleName == appIdMetaData.bundleName);
}

std::string AppIDMetaData::GetKey() const
{
    return appId;
}

bool AppIDMetaData::operator!=(const AppIDMetaData &appIdMetaData) const
{
    return !(*this == appIdMetaData);
}
} // namespace DistributedData
} // namespace OHOS


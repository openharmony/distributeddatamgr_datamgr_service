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

#include "metadata/corrupted_meta_data.h"

#include "utils/constant.h"
namespace OHOS {
namespace DistributedData {
bool CorruptedMetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(appId)], appId);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(storeId)], storeId);
    SetValue(node[GET_NAME(isCorrupted)], isCorrupted);
    return true;
}

bool CorruptedMetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(appId), appId);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(storeId), storeId);
    GetValue(node, GET_NAME(isCorrupted), isCorrupted);
    return true;
}

CorruptedMetaData::CorruptedMetaData()
{
}

CorruptedMetaData::~CorruptedMetaData()
{
}

CorruptedMetaData::CorruptedMetaData(
    const std::string &appId, const std::string &bundleName, const std::string &storeId)
    : appId(appId), bundleName(bundleName), storeId(storeId)
{
}

std::string CorruptedMetaData::GetKey()
{
    return Constant::Join(KEY_PREFIX, Constant::KEY_SEPARATOR,
        { appId, bundleName, storeId });
}
} // namespace DistributedData
} // namespace OHOS

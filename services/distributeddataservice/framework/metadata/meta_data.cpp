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

#include "metadata/meta_data.h"
namespace OHOS {
namespace DistributedData {
bool MetaData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(storeType)], storeType);
    SetValue(node[GET_NAME(storeMetaData)], storeMetaData);
    SetValue(node[GET_NAME(secretKeyMetaData)], secretKeyMetaData);
    return true;
}

bool MetaData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(storeType), storeType);
    GetValue(node, GET_NAME(storeMetaData), storeMetaData);
    GetValue(node, GET_NAME(secretKeyMetaData), secretKeyMetaData);
    return true;
}
} // namespace DistributedData
} // namespace OHOS

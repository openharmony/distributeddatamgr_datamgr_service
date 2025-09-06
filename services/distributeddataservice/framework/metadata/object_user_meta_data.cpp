/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "metadata/object_user_meta_data.h"

namespace OHOS::DistributedData {
ObjectUserMetaData::ObjectUserMetaData()
{
}
bool ObjectUserMetaData::Marshal(json &node) const
{
    return SetValue(node[GET_NAME(userId)], userId);
}

bool ObjectUserMetaData::Unmarshal(const json &node)
{
    return GetValue(node, GET_NAME(userId), userId);
}

std::string ObjectUserMetaData::GetKey()
{
    return KEY_PREFIX;
}

bool ObjectUserMetaData::operator==(const ObjectUserMetaData &meta) const
{
    return (userId == meta.userId);
}

bool ObjectUserMetaData::operator!=(const ObjectUserMetaData &meta) const
{
    return !(*this == meta);
}
}
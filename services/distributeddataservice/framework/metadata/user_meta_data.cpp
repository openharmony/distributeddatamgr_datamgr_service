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
#include "metadata/user_meta_data.h"

#include "utils/constant.h"

namespace OHOS::DistributedData {
constexpr const char *UserMetaRow::KEY_PREFIX;
bool UserMetaData::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(deviceId)], deviceId) && ret;
    ret = SetValue(node[GET_NAME(users)], users) && ret;

    return ret;
}

bool UserMetaData::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(deviceId), deviceId) && ret;
    ret = GetValue(node, GET_NAME(users), users) && ret;

    return ret;
}

bool UserStatus::Marshal(json &node) const
{
    bool ret = true;
    ret = SetValue(node[GET_NAME(id)], id) && ret;
    ret = SetValue(node[GET_NAME(isActive)], isActive) && ret;
    return ret;
}

bool UserStatus::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(id), id) && ret;
    ret = GetValue(node, GET_NAME(isActive), isActive) && ret;
    return ret;
}
UserStatus::UserStatus(int id, bool isActive) : id(id), isActive(isActive)
{
}

std::string UserMetaRow::GetKeyFor(const std::string &key)
{
    std::string str = Constant::Concatenate({ KEY_PREFIX, Constant::KEY_SEPARATOR, key });
    return { str.begin(), str.end() };
}
} // namespace OHOS::DistributedData

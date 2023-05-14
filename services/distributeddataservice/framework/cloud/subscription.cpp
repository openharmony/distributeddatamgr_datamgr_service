/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cloud/subscription.h"
#include "utils/constant.h"
namespace OHOS::DistributedData {
bool Subscription::Marshal(json &node) const
{
    SetValue(node[GET_NAME(userId)], userId);
    SetValue(node[GET_NAME(id)], id);
    SetValue(node[GET_NAME(expiresTime)], expiresTime);
    return true;
}

bool Subscription::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(userId), userId);
    GetValue(node, GET_NAME(id), id);
    GetValue(node, GET_NAME(expiresTime), expiresTime);
    return true;
}

std::string Subscription::GetKey()
{
    return GetKey(userId);
}

std::string Subscription::GetKey(int32_t userId)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, { std::to_string(userId) });
}

std::string Subscription::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData
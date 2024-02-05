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
bool Subscription::Relation::Marshal(json &node) const
{
    SetValue(node[GET_NAME(id)], id);
    SetValue(node[GET_NAME(bundleName)], bundleName);
    SetValue(node[GET_NAME(relations)], relations);
    return true;
}

bool Subscription::Relation::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(id), id);
    GetValue(node, GET_NAME(bundleName), bundleName);
    GetValue(node, GET_NAME(relations), relations);
    return true;
}

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

std::string Subscription::GetRelationKey(const std::string &bundleName)
{
    return GetRelationKey(userId, bundleName);
}

uint64_t Subscription::GetMinExpireTime() const
{
    if (expiresTime.empty()) {
        return 0;
    }
    auto it = expiresTime.begin();
    uint64_t expire = it->second;
    it++;
    for (; it != expiresTime.end(); it++) {
        if (it->second == 0) {
            continue;
        }
        if (it->second < expire) {
            expire = it->second;
        }
    }
    return expire;
}

std::string Subscription::GetKey(int32_t userId)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, { std::to_string(userId) });
}

std::string Subscription::GetRelationKey(int32_t userId, const std::string &bundleName)
{
    return Constant::Join(RELATION_PREFIX, Constant::KEY_SEPARATOR, { std::to_string(userId), bundleName });
}

std::string Subscription::GetPrefix(const std::initializer_list<std::string> &fields)
{
    return Constant::Join(PREFIX, Constant::KEY_SEPARATOR, fields);
}
} // namespace OHOS::DistributedData
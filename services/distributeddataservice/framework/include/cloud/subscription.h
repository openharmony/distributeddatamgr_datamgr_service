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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SUBSCRIPTION_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SUBSCRIPTION_H
#include "serializable/serializable.h"
namespace OHOS::DistributedData {
struct API_EXPORT Subscription final : public Serializable {
    int32_t userId = 0;
    std::string id;
    std::map<std::string, uint64_t> expiresTime;

    struct API_EXPORT Relation final : public Serializable {
        std::string id;
        std::string bundleName;
        std::map<std::string, std::string> relations;
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };

    bool Marshal(json &node) const;
    bool Unmarshal(const json &node);
    std::string GetKey();
    std::string GetRelationKey(const std::string &bundleName);
    uint64_t GetMinExpireTime() const;
    static std::string GetKey(int32_t userId);
    static std::string GetRelationKey(int32_t userId, const std::string &bundleName);
    static std::string GetPrefix(const std::initializer_list<std::string> &fields);
private:
    static constexpr const char *PREFIX = "CLOUD_SUBSCRIPTION";
    static constexpr const char *RELATION_PREFIX = "CLOUD_RELATION";
    static constexpr uint64_t INVALID_TIME = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SUBSCRIPTION_H

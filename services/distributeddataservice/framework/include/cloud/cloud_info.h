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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_INFO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_INFO_H
#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT CloudInfo final : public Serializable {
public:
    struct API_EXPORT AppInfo final : public Serializable {
        std::string bundleName = "";
        std::string appId = "";
        uint64_t version = 0;
        bool cloudSwitch = false;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };
    int32_t user = 0;
    std::string id = "";
    uint64_t totalSpace = 0;
    uint64_t remainSpace = 0;
    bool enableCloud = false;
    std::vector<AppInfo> apps;

    std::string GetKey() const;
    std::map<std::string, std::string> GetSchemaKey() const;
    std::string GetSchemaKey(std::string bundleName) const;
    bool IsValid() const;
    bool IsExist(const std::string &bundleName) const;
    static std::string GetPrefix(const std::initializer_list<std::string> &field);

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;

private:
    static constexpr const char *INFO_PREFIX = "CLOUD_INFO";
    static constexpr const char *SCHEMA_PREFIX = "CLOUD_SCHEMA";

    static std::string GetKey(const std::string &prefix, const std::initializer_list<std::string> &fields);
};
}
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_INFO_H

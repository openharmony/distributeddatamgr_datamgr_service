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
#include <optional>
#include "metadata/store_meta_data.h"
#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT CloudInfo final : public Serializable {
public:
    static constexpr int32_t DEFAULT_BATCH_NUMBER = 30;
    static constexpr int32_t DEFAULT_BATCH_SIZE = 1024 * 512 * 3; // 1.5M
    struct API_EXPORT AppInfo final : public Serializable {
        std::string bundleName = "";
        std::string appId = "";
        uint64_t version = 0;
        int32_t instanceId = 0;
        bool cloudSwitch = false;

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool operator==(const AppInfo &info) const;
        bool operator!=(const AppInfo &info) const;
    };
    int32_t user = 0;
    std::string id = "";
    uint64_t totalSpace = 0;
    uint64_t remainSpace = 0;
    int32_t maxNumber = 0;
    int32_t maxSize = 0;
    bool enableCloud = false;
    std::map<std::string, AppInfo> apps;

    std::string GetKey() const;
    std::map<std::string, std::string> GetSchemaKey() const;
    std::string GetSchemaKey(const std::string &bundleName, int32_t instanceId = 0) const;
    std::string GetSchemaPrefix(const std::string &bundleName) const;
    static std::string GetSchemaKey(int32_t user, const std::string &bundleName, int32_t instanceId = 0);
    static std::string GetSchemaKey(const StoreMetaData &meta);
    bool IsValid() const;
    bool Exist(const std::string &bundleName, int32_t instanceId = 0) const;
    bool IsOn(const std::string &bundleName, int32_t instanceId = 0) const;
    bool IsAllSwitchOff() const;
    static std::string GetPrefix(const std::initializer_list<std::string> &field);
    std::optional<AppInfo> GetAppInfo(const std::string &bundleName) const;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool IsMapEqual(const std::map<std::string, AppInfo> &appInfos) const;
    bool operator==(const CloudInfo &info) const;
    bool operator!=(const CloudInfo &info) const;

private:
    static constexpr const char *INFO_PREFIX = "CLOUD_INFO";
    static constexpr const char *SCHEMA_PREFIX = "CLOUD_SCHEMA";

    static std::string GetKey(const std::string &prefix, const std::initializer_list<std::string> &fields);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_INFO_H

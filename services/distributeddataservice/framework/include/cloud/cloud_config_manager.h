/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_CONFIG_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_CONFIG_MANAGER_H
#include <string>
#include <vector>
#include <map>
#include <shared_mutex>
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class CloudConfigManager {
public:
    struct Info {
        std::string localBundle;
        std::string cloudBundle;
    };
    API_EXPORT static CloudConfigManager &GetInstance();
    API_EXPORT void Initialize(const std::vector<Info> &mapper);
    API_EXPORT std::string ToLocal(const std::string &bundleName);
    API_EXPORT std::string ToCloud(const std::string &bundleName);

private:
    std::map<std::string, std::string> toLocalMapper_;
    std::map<std::string, std::string> toCloudMapper_;
};

} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_CONFIG_MANAGER_H

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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_LAST_SYNC_INFO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_LAST_SYNC_INFO_H

#include "serializable/serializable.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CloudLastSyncInfo final : public Serializable {
public:
    std::string id = "";
    std::string storeId = "";
    int64_t startTime = 0;
    int64_t finishTime = 0;
    int32_t code = -1;
    int32_t syncStatus = 0;
    int32_t instanceId = 0;
    API_LOCAL bool Marshal(json &node) const override;
    API_LOCAL bool Unmarshal(const json &node) override;
    static std::string GetKey(const int32_t user, const std::string &bundleName,
                              const std::string &storeId, int32_t instanceId = 0);
private:
    static constexpr const char *INFO_PREFIX = "CLOUD_LAST_SYNC_INFO";
    API_LOCAL static std::string GetKey(const std::string &prefix, const std::initializer_list<std::string> &fields);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_LAST_SYNC_INFO_H
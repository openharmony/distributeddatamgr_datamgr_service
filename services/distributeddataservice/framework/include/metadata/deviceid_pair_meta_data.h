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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_DEVICEID_PAIR_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_DEVICEID_PAIR_META_DATA_H
#include <string>

#include "serializable/serializable.h"
namespace OHOS::DistributedData {
class API_EXPORT DeviceIDMetaData final : public Serializable {
public:
    API_EXPORT DeviceIDMetaData();
    API_EXPORT ~DeviceIDMetaData();
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    API_EXPORT std::string GetKey() const;

    std::string currentUUID = "";
    std::string oldUUID = "";
private:
    static constexpr const char *KEY_PREFIX = "DeviceIDMeta";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_APPID_META_DATA_H
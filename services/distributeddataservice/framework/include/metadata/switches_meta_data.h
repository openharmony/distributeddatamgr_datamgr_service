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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SWITCHES_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SWITCHES_META_DATA_H

#include <string>
#include "serializable/serializable.h"

namespace OHOS::DistributedData {
class API_EXPORT SwitchesMetaData final : public Serializable {
public:
    static constexpr uint32_t DEFAULT_VERSION = 0;
    static constexpr uint32_t INVALID_VALUE = 0xFFFFFFFF;
    static constexpr uint16_t INVALID_LENGTH = 0;
    uint32_t version = DEFAULT_VERSION;
    uint32_t value = INVALID_VALUE;
    uint16_t length = INVALID_LENGTH;
    std::string deviceId;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    std::string GetKey() const;
    API_EXPORT bool operator==(const SwitchesMetaData &meta) const;
    API_EXPORT bool operator!=(const SwitchesMetaData &meta) const;
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);

private:
    static constexpr const char *KEY_PREFIX = "SwitchesMeta";
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SWITCHES_META_DATA_H

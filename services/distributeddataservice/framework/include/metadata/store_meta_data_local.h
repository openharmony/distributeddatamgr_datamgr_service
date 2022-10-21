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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_LOCAL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_LOCAL_H
#include <string>
#include <variant>
#include <vector>

#include "serializable/serializable.h"
namespace OHOS::DistributedData {
struct API_EXPORT PolicyValue final : public Serializable {
    uint32_t type = UINT32_MAX;
    uint32_t index = 0;
    uint32_t valueUint = 0;
    API_EXPORT PolicyValue() = default;
    API_EXPORT ~PolicyValue() = default;
    API_EXPORT bool IsValueEffect() const;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};
struct API_EXPORT StoreMetaDataLocal final : public Serializable {
    bool isAutoSync = false;
    bool isBackup = false;
    bool isDirty = false;
    bool isEncrypt = false;
    std::string dataDir = "";
    std::string schema = "";
    std::vector<PolicyValue> policies {};

    API_EXPORT StoreMetaDataLocal();
    API_EXPORT ~StoreMetaDataLocal();
    API_EXPORT bool operator==(const StoreMetaDataLocal &metaData) const;
    API_EXPORT bool operator!=(const StoreMetaDataLocal &metaData) const;
    API_EXPORT bool HasPolicy(uint32_t type);
    API_EXPORT PolicyValue GetPolicy(uint32_t type);
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    API_EXPORT static std::string GetKey(const std::initializer_list<std::string> &fields);
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);

private:
    static constexpr const char *KEY_PREFIX = "KvStoreMetaDataLocal";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_LOCAL_H

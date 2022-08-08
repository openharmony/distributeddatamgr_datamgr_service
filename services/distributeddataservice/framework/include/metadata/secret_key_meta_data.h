/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SECRET_KEY_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SECRET_KEY_META_DATA_H
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
struct API_EXPORT SecretKeyMetaData final : public Serializable {
    std::vector<uint8_t> time {};
    std::vector<uint8_t> sKey {};
    int32_t storeType = 0;
    API_EXPORT SecretKeyMetaData();
    API_EXPORT ~SecretKeyMetaData();
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;

    API_EXPORT static std::string GetKey(const std::initializer_list<std::string> &fields);
    API_EXPORT static std::string GetBackupKey(const std::initializer_list<std::string> &fields);
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);
    API_EXPORT static std::string GetBackupPrefix(const std::initializer_list<std::string> &fields);
private:
    static constexpr const char *KEY_PREFIX = "SecretKey";
    static constexpr const char *BACKUP_KEY_PREFIX = "BackupSecretKey";
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_SECRET_KEY_META_DATA_H

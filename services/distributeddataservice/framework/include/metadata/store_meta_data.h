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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H

#include <vector>

#include "serializable/serializable.h"

namespace OHOS::DistributedData {
struct StoreMetaData final : public Serializable {
    // record kvstore meta version for compatible, should update when modify kvstore meta structure.
    static constexpr uint32_t META_VERSION_SUPPORT_MULTIUSER = 0x03000002;
    static constexpr uint32_t META_VERSION_SUPPORT_MULTIUSER_HOS = 0x03000001;
    bool isAutoSync = false;
    bool isBackup = false;
    bool isDirty = false;
    bool isEncrypt = false;
    int32_t kvStoreType = 0;
    int32_t securityLevel = 0;
    int32_t uid = -1;
    std::string appId = "";
    std::string appType = "";
    std::string bundleName = "";
    std::string dataDir = "";
    std::string deviceAccountId = "";
    std::string deviceId = "";
    std::string schema = "";
    std::string storeId = "";
    std::string userId = "";
    uint32_t version = META_VERSION_SUPPORT_MULTIUSER;

    API_EXPORT ~StoreMetaData();
    API_EXPORT StoreMetaData();
    API_EXPORT StoreMetaData(const std::string &appId, const std::string &storeId, const std::string &userId);
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};
class KvStoreMetaRow {
public:
    KVSTORE_API static const std::string KEY_PREFIX;

    KVSTORE_API static std::vector<uint8_t> GetKeyFor(const std::string &key);
};

class SecretMetaRow {
public:
    KVSTORE_API static const std::string KEY_PREFIX;

    KVSTORE_API static std::vector<uint8_t> GetKeyFor(const std::string &key);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H

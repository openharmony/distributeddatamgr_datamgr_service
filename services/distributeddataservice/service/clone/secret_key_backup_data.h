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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_SECRET_KEY_BACKUP_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_SECRET_KEY_BACKUP_DATA_H
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
struct SecretKeyBackupData final : public Serializable {
public:
    struct BackupItem final : public Serializable {
        std::string bundleName;
        std::string dbName;
        std::string user;
        std::string sKey;
        int32_t instanceId;
        int32_t storeType;
        int32_t area = -1;
        std::vector<uint8_t> time;

        BackupItem();
        ~BackupItem();
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        bool IsValid() const;
    };
    std::vector<BackupItem> infos;
    SecretKeyBackupData();
    ~SecretKeyBackupData();
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_SECRET_KEY_BACKUP_DATA_H

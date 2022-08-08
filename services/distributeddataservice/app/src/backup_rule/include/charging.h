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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_APP_BACKUP_RULE_CHARGING_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_APP_BACKUP_RULE_CHARGING_H
#include "backuprule/backup_rule_manager.h"
namespace OHOS {
namespace DistributedData {
class Charging : public BackupRuleManager::BackupRule {
public:
    Charging() noexcept;
    ~Charging() {}
    bool CanBackup() override;
private:
    static Charging instance_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_APP_BACKUP_RULE_CHARGING_H

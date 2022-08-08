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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_BACKUPRULE_BACKUP_RULE_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_BACKUPRULE_BACKUP_RULE_MANAGER_H
#include <vector>
#include "visibility.h"
#include "concurrent_map.h"
namespace OHOS {
namespace DistributedData {
class BackupRuleManager {
public:
    class BackupRule {
    public:
        virtual bool CanBackup() = 0;
    protected:
        API_EXPORT ~BackupRule() = default;
    };
    API_EXPORT static BackupRuleManager &GetInstance();
    API_EXPORT void RegisterPlugin(const std::string &backupRule, std::function<BackupRule *()> getter);
    API_EXPORT void LoadBackupRules(const std::vector<std::string> &backupRules);
    API_EXPORT bool CanBackup();
private:
    std::vector<BackupRule *> backupRules_;
    ConcurrentMap<std::string, std::function<BackupRule *()>> getters_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_BACKUPRULE_BACKUP_RULE_MANAGER_H

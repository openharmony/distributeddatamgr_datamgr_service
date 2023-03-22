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
#include "backuprule/backup_rule_manager.h"
namespace OHOS::DistributedData {
BackupRuleManager &BackupRuleManager::GetInstance()
{
    static BackupRuleManager instance;
    return instance;
}

void BackupRuleManager::LoadBackupRules(const std::vector<std::string> &backupRules)
{
    for (const auto &backupRule : backupRules) {
        auto it = getters_.Find(backupRule);
        if (!it.first || it.second == nullptr) {
            continue;
        }
        auto *backupRuler = it.second();
        if (backupRuler == nullptr) {
            continue;
        }
        backupRules_.push_back(backupRuler);
    }
}

void BackupRuleManager::RegisterPlugin(const std::string &backupRule, std::function<BackupRule *()> getter)
{
    auto it = getters_.Find(backupRule);
    if (it.first) {
        return;
    }
    getters_.Compute(backupRule, [&getter](const auto &key, auto &value) {
        value = move(getter);
        return true;
    });
}

bool BackupRuleManager::CanBackup()
{
    for (auto rule : backupRules_) {
        if (rule == nullptr) {
            continue;
        }
        if (!rule->CanBackup()) {
            return false;
        }
    }
    return true;
}
} // namespace OHOS::DistributedData

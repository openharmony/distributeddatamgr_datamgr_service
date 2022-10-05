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
#define LOG_TAG "Bootstrap"
#include "bootstrap.h"

#include <dlfcn.h>
#include "backuprule/backup_rule_manager.h"
#include "backup_manager.h"
#include "checker/checker_manager.h"
#include "config_factory.h"
#include "directory_manager.h"
#include "log_print.h"
namespace OHOS {
namespace DistributedData {
Bootstrap &Bootstrap::GetInstance()
{
    static Bootstrap bootstrap;
    return bootstrap;
}

std::string Bootstrap::GetProcessLabel()
{
    auto *global = ConfigFactory::GetInstance().GetGlobalConfig();
    if (global == nullptr || global->processLabel.empty()) {
        return DEFAULT_LABEL;
    }
    return global->processLabel;
}

std::string Bootstrap::GetMetaDBName()
{
    auto *global = ConfigFactory::GetInstance().GetGlobalConfig();
    if (global == nullptr || global->metaData.empty()) {
        return DEFAULT_META;
    }
    return global->metaData;
}

void Bootstrap::LoadComponents()
{
    auto *comps = ConfigFactory::GetInstance().GetComponentConfig();
    if (comps == nullptr) {
        return;
    }
    for (auto &comp : *comps) {
        if (comp.lib.empty()) {
            continue;
        }

        // no need to close the component, so we don't keep the handles
        auto handle = dlopen(comp.lib.c_str(), RTLD_LAZY);
        if (handle == nullptr) {
            ZLOGE("dlopen(%{public}s) failed(%{public}d)!", comp.lib.c_str(), errno);
            continue;
        }

        if (comp.constructor.empty()) {
            continue;
        }

        auto ctor = reinterpret_cast<Constructor>(dlsym(handle, comp.constructor.c_str()));
        if (ctor == nullptr) {
            ZLOGE("dlsym(%{public}s) failed(%{public}d)!", comp.constructor.c_str(), errno);
            continue;
        }
        ctor(comp.params.c_str());
    }
}

void Bootstrap::LoadCheckers()
{
    auto *checkers = ConfigFactory::GetInstance().GetCheckerConfig();
    if (checkers == nullptr) {
        return;
    }
    CheckerManager::GetInstance().LoadCheckers(checkers->checkers);
    for (const auto &trust : checkers->trusts) {
        auto *checker = CheckerManager::GetInstance().GetChecker(trust.checker);
        if (checker == nullptr) {
            continue;
        }
        checker->SetTrustInfo(trust);
    }
}

void Bootstrap::LoadBackup()
{
    auto *backupRules = ConfigFactory::GetInstance().GetBackupConfig();
    if (backupRules == nullptr) {
        return;
    }
    BackupRuleManager::GetInstance().LoadBackupRules(backupRules->rules);

    BackupManager::BackupParam backupParam = { backupRules->schedularDelay,
        backupRules->schedularInternal, backupRules->backupInternal, backupRules->backupNumber};
    BackupManager::GetInstance().SetBackupParam(backupParam);
    BackupManager::GetInstance().Init();
    BackupManager::GetInstance().BackSchedule();
}

void Bootstrap::LoadNetworks()
{
}
void Bootstrap::LoadDirectory()
{
    auto *config = ConfigFactory::GetInstance().GetDirectoryConfig();
    if (config == nullptr) {
        return;
    }
    std::vector<DirectoryManager::Strategy> strategies(config->strategy.size());
    for (size_t i = 0; i < config->strategy.size(); ++i) {
        strategies[i] = config->strategy[i];
    }
    DirectoryManager::GetInstance().Initialize(strategies);
}
} // namespace DistributedData
} // namespace OHOS

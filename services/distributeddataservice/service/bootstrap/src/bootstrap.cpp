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

#include "access_check/app_access_check_config_manager.h"
#include "app_id_mapping/app_id_mapping_config_manager.h"
#include "backup_manager.h"
#include "backuprule/backup_rule_manager.h"
#include "checker/checker_manager.h"
#include "cloud/cloud_config_manager.h"
#include "config_factory.h"
#include "device_sync_app/device_sync_app_manager.h"
#include "directory/directory_manager.h"
#include "log_print.h"
#include "thread/thread_manager.h"
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
    for (const auto &distrust : checkers->distrusts) {
        auto *checker = CheckerManager::GetInstance().GetChecker(distrust.checker);
        if (checker == nullptr) {
            continue;
        }
        checker->SetDistrustInfo(distrust);
    }
    for (const auto &switches : checkers->switches) {
        auto *checker = CheckerManager::GetInstance().GetChecker(switches.checker);
        if (checker == nullptr) {
            continue;
        }
        checker->SetSwitchesInfo(switches);
    }
    for (const auto &dynamicStore : checkers->dynamicStores) {
        auto *checker = CheckerManager::GetInstance().GetChecker(dynamicStore.checker);
        if (checker == nullptr) {
            continue;
        }
        checker->AddDynamicStore(dynamicStore);
    }
    for (const auto &staticStore : checkers->staticStores) {
        auto *checker = CheckerManager::GetInstance().GetChecker(staticStore.checker);
        if (checker == nullptr) {
            continue;
        }
        checker->AddStaticStore(staticStore);
    }
}

void Bootstrap::LoadBackup(std::shared_ptr<ExecutorPool> executors)
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
    BackupManager::GetInstance().BackSchedule(std::move(executors));
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

void Bootstrap::LoadCloud()
{
    auto *config = ConfigFactory::GetInstance().GetCloudConfig();
    if (config == nullptr) {
        return;
    }
    std::vector<CloudConfigManager::Info> infos;
    for (auto &info : config->mapper) {
        infos.push_back({ info.localBundle, info.cloudBundle });
    }
    CloudConfigManager::GetInstance().Initialize(infos);
}

void Bootstrap::LoadAppIdMappings()
{
    auto *appIdMapping = ConfigFactory::GetInstance().GetAppIdMappingConfig();
    if (appIdMapping == nullptr) {
        return;
    }
    std::vector<AppIdMappingConfigManager::AppMappingInfo> infos;
    for (auto &info : *appIdMapping) {
        infos.push_back({ info.srcAppId, info.dstAppId });
    }
    AppIdMappingConfigManager::GetInstance().Initialize(infos);
}

void Bootstrap::LoadThread()
{
    auto *config = ConfigFactory::GetInstance().GetThreadConfig();
    if (config == nullptr) {
        return;
    }
    ThreadManager::GetInstance().Initialize(config->minThreadNum, config->maxThreadNum, config->ipcThreadNum);
}

void Bootstrap::LoadDeviceSyncAppWhiteLists()
{
    auto *deviceSyncAppWhiteLists = ConfigFactory::GetInstance().GetDeviceSyncAppWhiteListConfig();
    if (deviceSyncAppWhiteLists == nullptr) {
        return;
    }
    std::vector<DeviceSyncAppManager::WhiteList> infos;
    for (const auto &info : deviceSyncAppWhiteLists->whiteLists) {
        infos.push_back({ info.appId, info.bundleName, info.version });
    }
    DeviceSyncAppManager::GetInstance().Initialize(infos);
}

void Bootstrap::LoadSyncTrustedApp()
{
    auto *config = ConfigFactory::GetInstance().GetSyncAppsConfig();
    if (config == nullptr) {
        return;
    }
    std::vector<AppAccessCheckConfigManager::AppMappingInfo> infos;
    for (const auto &info : config->trusts) {
        infos.push_back({ info.bundleName, info.appId });
    }
    AppAccessCheckConfigManager::GetInstance().Initialize(infos);
}
} // namespace DistributedData
} // namespace OHOS
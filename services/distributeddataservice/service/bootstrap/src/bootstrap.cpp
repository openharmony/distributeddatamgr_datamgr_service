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

#include "bootstrap.h"

#include <dlfcn.h>

#include "checker/checker_manager.h"
#include "config_factory.h"
#include "directory_manager.h"
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

void Bootstrap::LoadComponents()
{
    auto *comps = ConfigFactory::GetInstance().GetComponentConfig();
    if (comps == nullptr) {
        return;
    }
    for (auto &comp : *comps) {
        auto handle = dlopen(comp.lib.c_str(), RTLD_LAZY);
        auto ctor = (Constructor)dlsym(handle, comp.constructor.c_str());
        if (ctor == nullptr) {
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

void Bootstrap::LoadNetworks()
{
}
void Bootstrap::LoadDirectory()
{
    auto *global = ConfigFactory::GetInstance().GetGlobalConfig();
    if (global == nullptr || global->directory == nullptr) {
        return;
    }
    for (const auto &strategy : global->directory->strategy) {
        DirectoryManager::GetInstance().AddParams(strategy);
    }
    DirectoryManager::GetInstance().SetCurrentVersion(global->directory->currentStrategyVersion);
}
} // namespace DistributedData
} // namespace OHOS

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
#include "checker/checker_manager.h"
namespace OHOS {
namespace DistributedData {
CheckerManager &CheckerManager::GetInstance()
{
    static CheckerManager instance;
    return instance;
}

void CheckerManager::LoadCheckers(std::vector<std::string> &checkers)
{
    for (const auto &checker : checkers) {
        if (checkers_.find(checker) != checkers_.end()) {
            continue;
        }
        auto it = getters_.Find(checker);
        if (!it.first || it.second == nullptr) {
            continue;
        }
        auto *bundleChecker = it.second();
        if (bundleChecker == nullptr) {
            continue;
        }
        bundleChecker->Initialize();
        checkers_[checker] = bundleChecker;
    }
}

void CheckerManager::RegisterPlugin(const std::string &checker, std::function<Checker *()> getter)
{
    auto it = getters_.Find(checker);
    if (it.first) {
        return;
    }
    getters_.Compute(checker, [&getter](const auto &key, auto &value) {
        value = move(getter);
        return true;
    });
}

std::string CheckerManager::GetAppId(const StoreInfo &info)
{
    for (auto &[name, checker] : checkers_) {
        if (checker == nullptr) {
            continue;
        }
        auto appId = checker->GetAppId(info);
        if (appId.empty()) {
            continue;
        }
        return appId;
    }
    return "";
}

bool CheckerManager::IsValid(const StoreInfo &info)
{
    for (auto &[name, checker] : checkers_) {
        if (checker == nullptr) {
            continue;
        }
        if (!checker->IsValid(info)) {
            continue;
        }
        return true;
    }
    return false;
}

CheckerManager::Checker *CheckerManager::GetChecker(const std::string &checker)
{
    auto it = checkers_.find(checker);
    if (it == checkers_.end()) {
        return nullptr;
    }
    return it->second;
}
} // namespace DistributedData
} // namespace OHOS
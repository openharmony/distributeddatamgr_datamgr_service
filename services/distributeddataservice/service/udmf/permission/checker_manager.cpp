/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "checker_manager.h"

namespace OHOS {
namespace UDMF {
const std::string DATA_CHECKER = "DataChecker";
CheckerManager &CheckerManager::GetInstance()
{
    static CheckerManager instance;
    return instance;
}

void CheckerManager::RegisterChecker(const std::string &checker, std::function<Checker *()> getter)
{
    getters_.ComputeIfAbsent(checker, [&getter](const auto &) {
        return move(getter);
    });
}

void CheckerManager::LoadCheckers()
{
    getters_.ForEach([this] (const auto &key, const auto &val) {
        if (this->checkers_.find(key) != this->checkers_.end()) {
            return false;
        }
        auto *checker = val();
        if (checker == nullptr) {
            return false;
        }
        this->checkers_[key] = checker;
        return false;
    });
}

bool CheckerManager::IsValid(const std::vector<Privilege> &privileges, const CheckInfo &info)
{
    auto it = checkers_.find(DATA_CHECKER);
    if (it == checkers_.end()) {
        return true;
    }
    if (it->second == nullptr) {
        return false;
    }
    return it->second->IsValid(privileges, info);
}
} // namespace UDMF
} // namespace OHOS

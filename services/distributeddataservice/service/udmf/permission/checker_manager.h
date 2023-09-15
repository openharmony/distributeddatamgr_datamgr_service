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

#ifndef UDMF_CHECKER_MANAGER_H
#define UDMF_CHECKER_MANAGER_H

#include <vector>

#include "concurrent_map.h"
#include "unified_types.h"

namespace OHOS {
namespace UDMF {
class CheckerManager {
public:
    struct CheckInfo {
        uint32_t tokenId;
    };

    class Checker {
    public:
        virtual bool IsValid(const std::vector<Privilege> &privileges, const CheckInfo &info) = 0;
    protected:
        ~Checker() = default;
    };

    static CheckerManager &GetInstance();

    void RegisterChecker(const std::string &checker, std::function<Checker *()> getter);
    void LoadCheckers();
    bool IsValid(const std::vector<Privilege> &privileges, const CheckInfo &info);

private:
    std::map<std::string, Checker *> checkers_;
    ConcurrentMap<std::string, std::function<Checker *()>> getters_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_CHECKER_MANAGER_H
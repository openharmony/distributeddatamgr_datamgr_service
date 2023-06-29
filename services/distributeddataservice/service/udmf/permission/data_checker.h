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

#ifndef UDMF_DATA_CHECKER_H
#define UDMF_DATA_CHECKER_H

#include "checker_manager.h"

namespace OHOS {
namespace UDMF {
class DataChecker : public CheckerManager::Checker {
public:
    DataChecker() noexcept;
    ~DataChecker();

    bool IsValid(const std::vector<Privilege> &privileges, const CheckerManager::CheckInfo &info) override;

private:
    static DataChecker instance_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DATA_CHECKER_H
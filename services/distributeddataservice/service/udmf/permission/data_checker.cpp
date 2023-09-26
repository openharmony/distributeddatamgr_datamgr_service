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
#define LOG_TAG "DataChecker"

#include "data_checker.h"

#include "utils/anonymous.h"
#include "log_print.h"

namespace OHOS {
namespace UDMF {
__attribute__((used)) DataChecker DataChecker::instance_;
DataChecker::DataChecker() noexcept
{
    CheckerManager::GetInstance().RegisterChecker(
        "DataChecker", [this]() -> auto { return this; });
}

DataChecker::~DataChecker()
{
}

bool DataChecker::IsValid(const std::vector<Privilege> &privileges, const CheckerManager::CheckInfo &info)
{
    for (const auto &privilege : privileges) {
        if (privilege.tokenId == info.tokenId) {
            return true;
        }
    }
    ZLOGE("Invalid parameters, %{public}s", DistributedData::Anonymous::Change(std::to_string(info.tokenId)).c_str());
    return false;
}
} // namespace UDMF
} // namespace OHOS
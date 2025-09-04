/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef DATASHARESERVICE_COMMON_UTILS_H
#define DATASHARESERVICE_COMMON_UTILS_H

#include <chrono>
#include <cinttypes>
#include <string>

namespace OHOS::DataShare {
struct DataShareThreadLocal {
    static bool& GetFromSystemApp();
    static void SetFromSystemApp(bool isFromSystemApp);
    static bool IsFromSystemApp();
    static void CleanFromSystemApp();
};

bool CheckSystemAbility(uint32_t tokenId);

bool CheckSystemCallingPermission(uint32_t tokenId, uint64_t fullTokenId);

inline int64_t GetSystemTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_COMMON_UTILS_H

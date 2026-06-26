/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_UTILS_SECURE_CLEAR_GUARD_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_UTILS_SECURE_CLEAR_GUARD_H

#include <string>
#include <type_traits>
#include <vector>
#include "securec.h"

namespace OHOS::DistributedData {

template<typename T>
class SecureClearGuard {
    static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<uint8_t>>
        || std::is_same_v<T, std::vector<char>>,
        "SecureClearGuard only supports std::string, std::vector<uint8_t>, or std::vector<char>");

public:
    explicit SecureClearGuard(T &data) : data_(data) {}
    ~SecureClearGuard()
    {
        if (!data_.empty()) {
            memset_s(data_.data(), data_.size(), 0, data_.size());
        }
    }

private:
    T &data_;
};

} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_UTILS_SECURE_CLEAR_GUARD_H

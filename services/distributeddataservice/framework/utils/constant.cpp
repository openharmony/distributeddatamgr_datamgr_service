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

#include "utils/constant.h"
#include <dirent.h>
#include <unistd.h>
#include <cerrno>

namespace OHOS {
namespace DistributedData {
constexpr const char *Constant::KEY_SEPARATOR;

std::string Constant::Concatenate(std::initializer_list<std::string> stringList)
{
    std::string result;
    size_t result_size = 0;
    for (const std::string &str : stringList) {
        result_size += str.size();
    }
    result.reserve(result_size);
    for (const std::string &str : stringList) {
        result.append(str.data(), str.size());
    }
    return result;
}
}  // namespace DistributedData
}  // namespace OHOS

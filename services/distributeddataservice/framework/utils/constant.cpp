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
#define LOG_TAG "Constant"
#include "utils/constant.h"

#include <cerrno>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include "log_print.h"

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

std::string Constant::Join(const std::string &prefix, const std::string &separator,
    std::initializer_list<std::string> params)
{
    std::string::size_type size = prefix.size();
    for (const std::string &param : params) {
        size += separator.size() + param.size();
    }
    std::string result;
    result.reserve(size);
    result.append(prefix);
    for (const std::string &str : params) {
        result.append(separator).append(str);
    }
    return result;
}

bool Constant::Equal(bool first, bool second)
{
    return (first && second) || (!first && !second);
}

bool Constant::NotEqual(bool first, bool second)
{
    return (first || second) && (!first || !second);
}

bool Constant::IsBackground(pid_t pid)
{
    std::ifstream ifs("/proc/" + std::to_string(pid) + "/cgroup", std::ios::in);
    ZLOGD("pid %d open %d", pid, ifs.good());
    if (!ifs.good()) {
        return false;
    }

    while (!ifs.eof()) {
        const int MAX_LEN = 256; // enough
        char buffer[MAX_LEN] = { 0 };
        ifs.getline(buffer, sizeof(buffer));
        std::string line = buffer;

        size_t pos = line.find("background");
        if (pos != std::string::npos) {
            return true;
        }
    }
    return false;
}
} // namespace DistributedData
} // namespace OHOS

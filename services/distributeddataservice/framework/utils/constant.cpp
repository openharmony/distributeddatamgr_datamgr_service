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
#include "log_print.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include <fstream>
#include "securec.h"

namespace OHOS {
namespace DistributedData {
constexpr const char *Constant::KEY_SEPARATOR;
static const std::string PATH_INVALID_FLAG_LEADING = "../";
static const std::string PATH_INVALID_FLAG_TRAILING = "/..";
static const uint32_t PATH_INVALID_FLAG_LEN = 3;
static const char FILE_SEPARATOR_CHAR = '/';

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

std::vector<std::string> Constant::Split(const std::string &str, const std::string &delim)
{
    if (str.empty()) {
        return { str };
    }
    std::vector<std::string> res;
    size_t pos = 0;
    while (pos < str.size()) {
        size_t found = str.find(delim, pos);
        if (found == std::string::npos) {
            res.push_back(str.substr(pos));
            break;
        }
        res.push_back(str.substr(pos, found - pos));
        pos = found + delim.size();
    }
    return res;
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
    return false;
}

bool Constant::DCopy(uint8_t *tag, size_t tagLen, const uint8_t *src, size_t srcLen)
{
    if (tagLen != srcLen || tag == nullptr || src == nullptr) {
        return false;
    }
    auto ret = memcpy_s(tag, tagLen, src, srcLen);
    return ret == EOK;
}

bool Constant::IsValidPath(const std::string &path)
{
    size_t pos = path.find(PATH_INVALID_FLAG_LEADING);
    while (pos != std::string::npos) {
        if (pos == 0 || path[pos - 1] == FILE_SEPARATOR_CHAR) {
            ZLOGE("leading is invalid. dataDir is %{public}s", Anonymous::Change(path).c_str());
            return false;
        }
        pos = path.find(PATH_INVALID_FLAG_LEADING, pos + PATH_INVALID_FLAG_LEN);
    }
    pos = path.rfind(PATH_INVALID_FLAG_TRAILING);
    if ((pos != std::string::npos) && (path.size() - pos == PATH_INVALID_FLAG_LEN)) {
        ZLOGE("trailing is invalid. dataDir is %{public}s", Anonymous::Change(path).c_str());
        return false;
    }
    return true;
}
} // namespace DistributedData
} // namespace OHOS

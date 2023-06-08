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

#ifndef KV_DATASERVICE_CONSTANT_H
#define KV_DATASERVICE_CONSTANT_H

#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <type_traits>
#include <vector>
#include "visibility.h"

namespace OHOS {
namespace DistributedData {
class Constant {
public:
    // concatenate strings and return a composition string.
    API_EXPORT static std::string Concatenate(std::initializer_list<std::string> stringList);

    API_EXPORT static std::string Join(
        const std::string &prefix, const std::string &separator, std::initializer_list<std::string> params);

    API_EXPORT static bool IsBackground(pid_t pid);

    API_EXPORT static bool Equal(bool first, bool second);

    API_EXPORT static bool NotEqual(bool first, bool second);

    template<typename T>
    inline static constexpr bool is_pod = (std::is_standard_layout_v<T> && std::is_trivial_v<T>);

    template<typename T, typename S>
    API_EXPORT inline static std::enable_if_t<is_pod<T> && is_pod<S>, bool> Copy(T *tag, const S *src)
    {
        return DCopy(reinterpret_cast<uint8_t *>(tag), sizeof(T), reinterpret_cast<const uint8_t *>(src), sizeof(S));
    };

    // delete left bland in s by reference.
    template<typename T>
    static void LeftTrim(T &s);

    // delete right bland in s by reference.
    template<typename T>
    static void RightTrim(T &s);

    // delete both left and right bland in s by reference.
    template<typename T>
    static void Trim(T &s);

    // delete both left and right bland in s by reference, not change raw string.
    template<typename T>
    static T TrimCopy(T s);

    API_EXPORT static constexpr const char *KEY_SEPARATOR = "###";

private:
    API_EXPORT static bool DCopy(uint8_t *tag, size_t tagLen, const uint8_t *src, size_t srcLen);
};

// trim from start (in place)
template<typename T>
void Constant::LeftTrim(T &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
template<typename T>
void Constant::RightTrim(T &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
template<typename T>
void Constant::Trim(T &s)
{
    LeftTrim(s);
    RightTrim(s);
}

// trim from both ends (copying)
template<typename T>
T Constant::TrimCopy(T s)
{
    Trim(s);
    return s;
}
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KV_DATASERVICE_CONSTANT_H

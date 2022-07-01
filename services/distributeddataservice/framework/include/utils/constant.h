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

#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
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

    // delete left bland in s by reference.
    template<typename T>
    static void LeftTrim(T &s);

    // delete right bland in s by reference.
    template<typename T>
    static void RightTrim(T &s);

    // delete both left and right bland in s by reference.
    template<typename T>
    static void Trim(T &s);

    // delete left bland in s by reference, not change raw string.
    template<typename T>
    static T LeftTrimCopy(T s);

    // delete right bland in s by reference, not change raw string.
    template<typename T>
    static T RightTrimCopy(T s);

    // delete both left and right bland in s by reference, not change raw string.
    template<typename T>
    static T TrimCopy(T s);

    API_EXPORT static constexpr const char *KEY_SEPARATOR = "###";
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

// trim from start (copying)
template<typename T>
T Constant::LeftTrimCopy(T s)
{
    LeftTrim(s);
    return s;
}

// trim from end (copying)
template<typename T>
T Constant::RightTrimCopy(T s)
{
    RightTrim(s);
    return s;
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

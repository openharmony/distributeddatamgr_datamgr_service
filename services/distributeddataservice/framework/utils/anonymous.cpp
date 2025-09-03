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

#include "utils/anonymous.h"
#include <vector>
namespace OHOS {
namespace DistributedData {

constexpr int32_t CONTINUOUS_DIGITS_MINI_SIZE = 6;

std::string AnonyDigits(const std::string &fileName)
{
    std::string::size_type digitsNum = fileName.size();
    if (digitsNum < CONTINUOUS_DIGITS_MINI_SIZE) {
        return fileName;
    }
    std::string::size_type endDigitsNum = 4;
    std::string::size_type shortEndDigitsNum = 3;
    std::string name = fileName;
    std::string last = "";
    if (digitsNum == CONTINUOUS_DIGITS_MINI_SIZE) {
        last = name.substr(name.size() - shortEndDigitsNum);
    } else {
        last = name.substr(name.size() - endDigitsNum);
    }

    return "***" + last;
}

std::string Anonymous::Change(const std::string &name)
{
    std::vector<std::string> alnum;
    std::vector<std::string> noAlnum;
    std::string alnumStr;
    std::string noAlnumStr;
    for (const auto &letter : name) {
        if (isxdigit(letter)) {
            if (!noAlnumStr.empty()) {
                noAlnum.push_back(noAlnumStr);
                noAlnumStr.clear();
                alnum.push_back("");
            }
            alnumStr += letter;
        } else {
            if (!alnumStr.empty()) {
                alnum.push_back(alnumStr);
                alnumStr.clear();
                noAlnum.push_back("");
            }
            noAlnumStr += letter;
        }
    }
    if (!alnumStr.empty()) {
        alnum.push_back(alnumStr);
        noAlnum.push_back("");
    }
    if (!noAlnumStr.empty()) {
        noAlnum.push_back(noAlnumStr);
        alnum.push_back("");
    }
    std::string res = "";
    for (size_t i = 0; i < alnum.size(); ++i) {
        res += (AnonyDigits(alnum[i]) + noAlnum[i]);
    }
    return res;
}
} // namespace DistributedData
} // namespace OHOS

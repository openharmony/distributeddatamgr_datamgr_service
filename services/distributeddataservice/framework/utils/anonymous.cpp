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

constexpr int32_t HEAD_SIZE = 3;
constexpr int32_t END_SIZE = 3;
constexpr int32_t MIN_SIZE = HEAD_SIZE + END_SIZE + 3;
constexpr const char *REPLACE_CHAIN = "***";
constexpr int32_t FILE_PATH_MINI_SIZE = 6;
constexpr int32_t AREA_MINI_SIZE = 4;
constexpr int32_t AREA_OFFSET_SIZE = 5;
constexpr int32_t PRE_OFFSET_SIZE = 1;

std::string AnonymousName(const std::string &fileName)
{
    if (fileName.empty()) {
        return "";
    }

    if (fileName.length() <= HEAD_SIZE) {
        return fileName.substr(0, 1) + "**";
    }

    if (fileName.length() < MIN_SIZE) {
        return (fileName.substr(0, HEAD_SIZE) + REPLACE_CHAIN);
    }

    return (fileName.substr(0, HEAD_SIZE) + REPLACE_CHAIN + fileName.substr(fileName.length() - END_SIZE, END_SIZE));
}

std::string Anonymous::Change(const std::string &name)
{
    auto pre = name.find("/");
    auto end = name.rfind("/");
    if (pre == std::string::npos || end - pre < FILE_PATH_MINI_SIZE) {
        return AnonymousName(name);
    }
    auto path = name.substr(pre, end - pre);
    auto area = path.find("/el");
    if (area == std::string::npos || area + AREA_MINI_SIZE > path.size()) {
        path = "";
    } else if (area + AREA_OFFSET_SIZE < path.size()) {
        path = path.substr(area, AREA_MINI_SIZE) + "/***";
    } else {
        path = path.substr(area, AREA_MINI_SIZE);
    }
    std::string fileName = name.substr(end + 1);
    fileName = AnonymousName(fileName);
    return name.substr(0, pre + PRE_OFFSET_SIZE) + "***" + path + "/" + fileName;
}
} // namespace DistributedData
} // namespace OHOS

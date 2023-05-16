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
#include "os_api.h"

#include <climits>
#include <sys/stat.h>
#include <unistd.h>

#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
namespace {
const int ACCESS_MODE_EXISTENCE = 0;
}
namespace OSAPI {
bool CheckPermission(const std::string &filePath)
{
    return (access(filePath.c_str(), R_OK) == 0) && (access(filePath.c_str(), W_OK) == 0);
}

bool CheckPathExistence(const std::string &filePath)
{
    return (access(filePath.c_str(), ACCESS_MODE_EXISTENCE) == 0);
}

int GetRealPath(const std::string &inOriPath, std::string &outRealPath)
{
    const unsigned int MAX_PATH_LENGTH = PATH_MAX;
    if (inOriPath.length() > MAX_PATH_LENGTH) { // max limit is 64K(0x10000).
        GLOGE("[OS_API] OriPath too long.");
        return -E_INVALID_ARGS;
    }

    char *realPath = new (std::nothrow) char[MAX_PATH_LENGTH + 1];
    if (realPath == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    if (memset_s(realPath, MAX_PATH_LENGTH + 1, 0, MAX_PATH_LENGTH + 1) != EOK) {
        delete[] realPath;
        return -E_SECUREC_ERROR;
    }

    if (realpath(inOriPath.c_str(), realPath) == nullptr) {
        GLOGE("[OS_API] Realpath error:%d.", errno);
        delete[] realPath;
        return -E_SYSTEM_API_FAIL;
    }
    outRealPath = std::string(realPath);
    delete[] realPath;
    return E_OK;
}

void SplitFilePath(const std::string &filePath, std::string &fieldir, std::string &fileName)
{
    if (filePath.empty()) {
        return;
    }

    auto slashPos = filePath.find_last_of('/');
    if (slashPos == std::string::npos) {
        fileName = filePath;
        fieldir = "";
        return;
    }

    fieldir = filePath.substr(0, slashPos);
    fileName = filePath.substr(slashPos + 1);
}
} // namespace OSAPI
} // namespace DocumentDB
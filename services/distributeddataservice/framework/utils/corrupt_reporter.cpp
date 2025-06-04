/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "CorruptReporter"
#include "utils/corrupt_reporter.h"

#include "log_print.h"
#include "utils/anonymous.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace OHOS::DistributedData {
constexpr const char *DB_CORRUPTED_POSTFIX = ".corruptedflg";

bool CorruptReporter::CreateCorruptedFlag(const std::string &path, const std::string &dbName)
{
    if (path.empty() || dbName.empty()) {
        ZLOGW("The path or dbName is empty, path:%{public}s, dbName:%{public}s", path.c_str(),
            Anonymous::Change(dbName).c_str());
        return false;
    }
    std::string flagFileName = path + "/" + dbName + DB_CORRUPTED_POSTFIX;
    if (access(flagFileName.c_str(), F_OK) == 0) {
        return true;
    }
    int fd = creat(flagFileName.c_str(), S_IRUSR | S_IWUSR);
    if (fd == -1) {
        ZLOGW("Create corrupted flag fail, flagFileName:%{public}s, errno:%{public}d",
            Anonymous::Change(flagFileName).c_str(), errno);
        return false;
    }
    close(fd);
    return true;
}

bool CorruptReporter::HasCorruptedFlag(const std::string &path, const std::string &dbName)
{
    if (path.empty() || dbName.empty()) {
        ZLOGW("The path or dbName is empty, path:%{public}s, dbName:%{public}s", path.c_str(),
            Anonymous::Change(dbName).c_str());
        return false;
    }
    std::string flagFileName = path + "/" + dbName + DB_CORRUPTED_POSTFIX;
    return access(flagFileName.c_str(), F_OK) == 0;
}

bool CorruptReporter::DeleteCorruptedFlag(const std::string &path, const std::string &dbName)
{
    if (path.empty() || dbName.empty()) {
        ZLOGW("The path or dbName is empty, path:%{public}s, dbName:%{public}s", path.c_str(),
            Anonymous::Change(dbName).c_str());
        return false;
    }
    std::string flagFileName = path + "/" + dbName + DB_CORRUPTED_POSTFIX;
    int result = remove(flagFileName.c_str());
    if (result != 0) {
        ZLOGW("Remove corrupted flag fail, flagFileName:%{public}s, errno:%{public}d",
            Anonymous::Change(flagFileName).c_str(), errno);
        return false;
    }
    return true;
}
} // namespace OHOS::DistributedData

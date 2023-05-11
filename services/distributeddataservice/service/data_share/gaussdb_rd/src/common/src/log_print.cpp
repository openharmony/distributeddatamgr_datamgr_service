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

#include <log_print.h>

#include "hilog/log.h"
#include "securec.h"

namespace DocumentDB {
namespace {

void PrintLog(Logger::Level level, const char *tag, const std::string &msg)
{
    if (msg.empty()) {
        return;
    }
    const std::string format = "%{public}s";
    OHOS::HiviewDFX::HiLogLabel label = { LOG_CORE, 0xD001631, tag };
    switch (level) {
        case Logger::Level::LEVEL_DEBUG:
            (void)OHOS::HiviewDFX::HiLog::Debug(label, format.c_str(), msg.c_str());
            break;
        case Logger::Level::LEVEL_INFO:
            (void)OHOS::HiviewDFX::HiLog::Info(label, format.c_str(), msg.c_str());
            break;
        case Logger::Level::LEVEL_WARN:
            (void)OHOS::HiviewDFX::HiLog::Warn(label, format.c_str(), msg.c_str());
            break;
        case Logger::Level::LEVEL_ERROR:
            (void)OHOS::HiviewDFX::HiLog::Error(label, format.c_str(), msg.c_str());
            break;
        case Logger::Level::LEVEL_FATAL:
            (void)OHOS::HiviewDFX::HiLog::Fatal(label, format.c_str(), msg.c_str());
            break;
        default:
            break;
    }
}

void PreparePrivateLog(const char *format, std::string &outStrFormat)
{
    static const std::string PRIVATE_TAG = "s{private}";
    outStrFormat = format;
    std::string::size_type pos = outStrFormat.find(PRIVATE_TAG);
    if (pos != std::string::npos) {
        outStrFormat.replace(pos, PRIVATE_TAG.size(), ".3s");
    }
}
} // namespace

void Logger::Log(Level level, const char *tag, const char *format, ...)
{
    static const int maxLogLength = 1024;

    va_list argList;
    va_start(argList, format);
    char logBuff[maxLogLength];
    std::string msg;
    std::string formatTemp;
    PreparePrivateLog(format, formatTemp);
    int bytes = vsnprintf_s(logBuff, maxLogLength, maxLogLength - 1, formatTemp.c_str(), argList);
    if (bytes < 0) {
        msg = "log buffer overflow!";
    } else {
        msg = logBuff;
    }
    va_end(argList);

    PrintLog(level, tag, msg);
}
} // namespace DocumentDB

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

#ifndef LOG_PRINT_H
#define LOG_PRINT_H

#include <string>

namespace DocumentDB {
constexpr const char *LOG_TAG_DOC = "GAUSSDB_RD";

class Logger {
public:
    enum class Level {
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARN,
        LEVEL_ERROR,
        LEVEL_FATAL
    };

    static void Log(Level level, const std::string &tag, const char *format, ...);
};
} // namespace DocumentDB

#define NO_LOG(...) // No log in normal and release. Used for convenience when deep debugging
#define GLOGD(...) Logger::Log(Logger::Level::LEVEL_DEBUG, LOG_TAG_DOC, __VA_ARGS__)
#define GLOGI(...) Logger::Log(Logger::Level::LEVEL_INFO, LOG_TAG_DOC, __VA_ARGS__)
#define GLOGW(...) Logger::Log(Logger::Level::LEVEL_WARN, LOG_TAG_DOC, __VA_ARGS__)
#define GLOGE(...) Logger::Log(Logger::Level::LEVEL_ERROR, LOG_TAG_DOC, __VA_ARGS__)
#define GLOGF(...) Logger::Log(Logger::Level::LEVEL_FATAL, LOG_TAG_DOC, __VA_ARGS__)
#endif // LOG_PRINT_H
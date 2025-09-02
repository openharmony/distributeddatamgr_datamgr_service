/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "TimeUtils"

#include "utils/time_utils.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace OHOS {
namespace DistributedData {
constexpr int MAX_TIME_BUF_LEN = 32;
constexpr int MILLISECONDS_LEN = 3;
constexpr int NANO_TO_MILLI = 1000000;
constexpr int MILLI_PRE_SEC = 1000;

std::string TimeUtils::GetCurSysTimeWithMs()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::chrono::nanoseconds nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
    return GetTimeWithMs(time, nsec.count());
}

std::string TimeUtils::GetTimeWithMs(time_t sec, int64_t nsec)
{
    std::stringstream oss;
    char buffer[MAX_TIME_BUF_LEN] = { 0 };
    std::tm local_time;
    localtime_r(&sec, &local_time);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    oss << buffer << '.' << std::setfill('0') << std::setw(MILLISECONDS_LEN) << (nsec / NANO_TO_MILLI) % MILLI_PRE_SEC;
    return oss.str();
}
} // namespace DistributedData
} // namespace OHOS

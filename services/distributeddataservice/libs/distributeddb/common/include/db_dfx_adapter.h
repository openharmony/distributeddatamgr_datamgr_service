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

#ifndef DB_DFX_ADAPTER_H
#define DB_DFX_ADAPTER_H

#include <string>
#include <vector>

namespace DistributedDB {
enum DBEventType {
    FAULT = 1,
    STATISTIC = 2,
    SECURITY = 3,
    BEHAVIOR = 4
};
struct ReportTask {
    std::string eventName;
    std::string appId;
    std::string userId;
    std::string storeId;
    int errCode = 0;
};
class DBDfxAdapter {
public:
    static void Dump(int fd, const std::vector<std::u16string> &args);

    static void ReportFault(const ReportTask &reportTask);

    static void StartTrace(const std::string &action);
    static void FinishTrace();

    static void StartTraceSQL();
    static void FinishTraceSQL();

    static void StartAsyncTrace(const std::string &action, int32_t taskId);
    static void FinishAsyncTrace(const std::string &action, int32_t taskId);

    static const std::string SYNC_ACTION;
    static const std::string EVENT_OPEN_DATABASE_FAILED;
private:
    static const std::string EVENT_CODE;
    static const std::string APP_ID;
    static const std::string USER_ID;
    static const std::string STORE_ID;
    static const std::string SQLITE_EXECUTE;
};
} // namespace DistributedDB

#endif // DB_DFX_ADAPTER_H

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
#include "db_dfx_adapter.h"

#include <codecvt>
#include <cstdio>
#include <locale>
#include <string>

#include "log_print.h"
#include "db_dump_helper.h"
#include "db_errno.h"
#include "kvdb_manager.h"
#include "relational_store_instance.h"
#include "runtime_context.h"
#include "sqlite_utils.h"
#ifdef USE_DFX_ABILITY
#include "hitrace_meter.h"
#include "hisysevent.h"
#endif

namespace DistributedDB {
namespace {
#ifdef USE_DFX_ABILITY
constexpr uint64_t HITRACE_LABEL = HITRACE_TAG_DISTRIBUTEDDATA;
#endif
constexpr const char *DUMP_PARAM = "dump-distributeddb";
}

const std::string DBDfxAdapter::EVENT_CODE = "ERROR_CODE";
const std::string DBDfxAdapter::APP_ID = "APP_ID";
const std::string DBDfxAdapter::USER_ID = "USER_ID";
const std::string DBDfxAdapter::STORE_ID = "STORE_ID";
const std::string DBDfxAdapter::SQLITE_EXECUTE = "SQLITE_EXECUTE";
const std::string DBDfxAdapter::SYNC_ACTION = "SYNC_ACTION";
const std::string DBDfxAdapter::EVENT_OPEN_DATABASE_FAILED = "OPEN_DATABASE_FAILED";

void DBDfxAdapter::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (!args.empty()) {
        const std::u16string u16DumpParam =
            std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(DUMP_PARAM);
        auto find = std::any_of(args.begin(), args.end(), [&u16DumpParam](const std::u16string &arg) {
            return arg == u16DumpParam;
        });
        if (!find) {
            return;
        }
    }
    DBDumpHelper::Dump(fd, "DistributedDB Dump Message Info:\n\n");
    DBDumpHelper::Dump(fd, "DistributedDB Database Basic Message Info:\n");
    KvDBManager::GetInstance()->Dump(fd);
    RelationalStoreInstance::GetInstance()->Dump(fd);
    DBDumpHelper::Dump(fd, "DistributedDB Common Message Info:\n");
    RuntimeContext::GetInstance()->DumpCommonInfo(fd);
    DBDumpHelper::Dump(fd, "\tlast error msg = %s\n", SQLiteUtils::GetLastErrorMsg().c_str());
}

#ifdef USE_DFX_ABILITY
void DBDfxAdapter::ReportFault(const ReportTask &reportTask)
{
    RuntimeContext::GetInstance()->ScheduleTask([=]() {
        // call hievent here
        OHOS::HiviewDFX::HiSysEvent::Write(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            reportTask.eventName,
            OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
            APP_ID, reportTask.appId,
            STORE_ID, reportTask.storeId,
            EVENT_CODE, std::to_string(reportTask.errCode));
    });
}

void DBDfxAdapter::StartTrace(const std::string &action)
{
    ::StartTrace(HITRACE_LABEL, action);
}

void DBDfxAdapter::FinishTrace()
{
    ::FinishTrace(HITRACE_LABEL);
}

void DBDfxAdapter::StartTraceSQL()
{
#ifdef TRACE_SQLITE_EXECUTE
    ::StartTrace(HITRACE_LABEL, SQLITE_EXECUTE);
#endif
}

void DBDfxAdapter::FinishTraceSQL()
{
#ifdef TRACE_SQLITE_EXECUTE
    ::FinishTrace(HITRACE_LABEL);
#endif
}

void DBDfxAdapter::StartAsyncTrace(const std::string &action, int32_t taskId)
{
    // call hitrace here
    // need include bytrace.h
    ::StartAsyncTrace(HITRACE_LABEL, action, taskId);
}

void DBDfxAdapter::FinishAsyncTrace(const std::string &action, int32_t taskId)
{
    // call hitrace here
    ::FinishAsyncTrace(HITRACE_LABEL, action, taskId);
}

#else
void DBDfxAdapter::ReportFault(const ReportTask &reportTask)
{
    (void) reportTask;
}

void DBDfxAdapter::StartTrace(const std::string &action)
{
    (void) action;
}

void DBDfxAdapter::FinishTrace()
{
}

void DBDfxAdapter::StartAsyncTrace(const std::string &action, int32_t taskId)
{
    (void) action;
    (void) taskId;
}

void DBDfxAdapter::FinishAsyncTrace(const std::string &action, int32_t taskId)
{
    (void) action;
    (void) taskId;
}

void DBDfxAdapter::StartTraceSQL()
{
}

void DBDfxAdapter::FinishTraceSQL()
{
}
#endif
} // namespace DistributedDB
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
static constexpr uint64_t HITRACE_LABEL = HITRACE_TAG_DISTRIBUTEDDATA;
#endif
static const std::string DUMP_PARAM = "dump-distributeddb";
}
std::atomic<bool> DBDfxAdapter::shutdown_ = false;
std::thread DBDfxAdapter::reportThread_ = std::thread(&DBDfxAdapter::ReportRoutine);

const std::string DBDfxAdapter::EVENT_CODE = "ERROR_CODE";
const std::string DBDfxAdapter::APP_ID = "APP_ID";
const std::string DBDfxAdapter::USER_ID = "USER_ID";
const std::string DBDfxAdapter::STORE_ID = "STORE_ID";
const std::string DBDfxAdapter::SQLITE_EXECUTE = "SQLITE_EXECUTE";
const std::string DBDfxAdapter::SYNC_ACTION = "SYNC_ACTION";
const std::string DBDfxAdapter::EVENT_OPEN_DATABASE_FAILED = "OPEN_DATABASE_FAILED";
std::mutex DBDfxAdapter::reprotTaskQueueMutex_;
std::queue<ReportTask> DBDfxAdapter::reportTaskQueue_;

bool DBDfxAdapter::wakingSignal_ = false;
std::mutex DBDfxAdapter::waitMutex_;
std::condition_variable DBDfxAdapter::waitingCv_;

void DBDfxAdapter::Finalize()
{
    shutdown_ = true;
    {
        std::lock_guard<std::mutex> autoLock(waitMutex_);
        wakingSignal_ = true;
    }
    waitingCv_.notify_one();
    reportThread_.join();
}

void DBDfxAdapter::Dump(int fd, const std::vector<std::u16string> &args)
{
#ifdef RUNNING_ON_LINUX
    static const std::u16string u16DumpParam =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(DUMP_PARAM);
    bool abort = true;
    for (auto &arg : args) {
        if (u16DumpParam == arg) {
            abort = false;
            break;
        }
    }
    if (abort) {
        return;
    }
    dprintf(fd, "DistributedDB Dump Message Info:\n\n");
    dprintf(fd, "DistributedDB Database Basic Message Info:");
    KvDBManager::GetInstance()->Dump(fd);
    RelationalStoreInstance::GetInstance()->Dump(fd);
    dprintf(fd, "DistributedDB Common Message Info:\n\n");
    RuntimeContext::GetInstance()->DumpCommonInfo(fd);
    dprintf(fd, "\tlast error msg = %s\n", SQLiteUtils::GetLastErrorMsg().c_str());
#else
    (void) fd;
    (void) args;
#endif
}

#ifdef USE_DFX_ABILITY
void DBDfxAdapter::ReportFault(const ReportTask &reportTask)
{
    if (shutdown_) {
        return;
    }
    std::lock_guard<std::mutex> autoLock(reprotTaskQueueMutex_);
    reportTaskQueue_.push(reportTask);
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

void DBDfxAdapter::ReportRoutine()
{
    while (!shutdown_) {
        if (IsReprotTaskEmpty()) {
            std::unique_lock<std::mutex> waitingLock(waitMutex_);
            LOGI("[DBDfxAdapter][ReportRoutine] Report done and sleep");
            waitingCv_.wait(waitingLock, [] { return wakingSignal_; });
            LOGI("[DBDfxAdapter][ReportRoutine] Report continue");
            wakingSignal_ = false;
        }
        ReportTask reportTask;
        if (ScheduleOutTask(reportTask) == -E_FINISHED) {
            continue;
        }

        // call hievent here
        OHOS::HiviewDFX::HiSysEvent::Write(OHOS::HiviewDFX::HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            reportTask.eventName,
            OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
            EVENT_CODE, std::to_string(reportTask.errCode));
    }
}

bool DBDfxAdapter::IsReprotTaskEmpty()
{
    std::lock_guard<std::mutex> autoLock(reprotTaskQueueMutex_);
    return reportTaskQueue_.empty();
}

int DBDfxAdapter::ScheduleOutTask(ReportTask &reportTask)
{
    std::lock_guard<std::mutex> autoLock(reprotTaskQueueMutex_);
    if (reportTaskQueue_.empty()) {
        return -E_FINISHED;
    }
    reportTask = reportTaskQueue_.back();
    reportTaskQueue_.pop();
    return -E_UNFINISHED;
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

void DBDfxAdapter::ReportRoutine()
{
}

bool DBDfxAdapter::IsReprotTaskEmpty()
{
    return true;
}

int DBDfxAdapter::ScheduleOutTask(ReportTask &reportTask)
{
    (void) reportTask;
    return 0;
}

void DBDfxAdapter::StartTraceSQL()
{
}

void DBDfxAdapter::FinishTraceSQL()
{
}
#endif

} // namespace DistributedDB
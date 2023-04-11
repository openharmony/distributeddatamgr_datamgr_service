/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#include "dump_helper.h"
#include "kvstore_data_service.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
namespace {
constexpr int32_t MAX_RECORED_ERROR = 10;
constexpr int32_t SUB_CMD_NAME = 0;
constexpr int32_t SUB_CMD_PARAM = 1;
constexpr int32_t CMD_NO_PARAM = 1;
constexpr int32_t CMD_HAS_PARAM = 2;
constexpr const char *CMD_HELP = "-h";
constexpr const char *CMD_USER_INFO = "-userInfo";
constexpr const char *CMD_APP_INFO = "-appInfo";
constexpr const char *CMD_STORE_INFO = "-storeInfo";
constexpr const char *CMD_ERROR_INFO = "-errorInfo";
constexpr const char *ILLEGAL_INFOMATION = "The arguments are illegal and you can enter '-h' for help.\n";
}

void DumpHelper::AddDumpOperation(const DumpNoParamFunc &dumpAll, const DumpNoParamFunc &dumpUserInfo,
    const DumpWithParamFunc &dumpAppInfo, const DumpWithParamFunc &dumpStoreInfo)
{
    if (dumpAll == nullptr || dumpUserInfo == nullptr || dumpAppInfo == nullptr || dumpStoreInfo == nullptr) {
        return;
    }
    dumpAll_ = dumpAll;
    dumpUserInfo_ = dumpUserInfo;
    dumpAppInfo_ = dumpAppInfo;
    dumpStoreInfo_ = dumpStoreInfo;
}

void DumpHelper::AddErrorInfo(const std::string &error)
{
    std::lock_guard<std::mutex> lock(hidumperMutex_);
    if (g_errorInfo.size() + 1 > MAX_RECORED_ERROR) {
        g_errorInfo.pop_front();
        g_errorInfo.push_back(error);
    } else {
        g_errorInfo.push_back(error);
    }
}

void DumpHelper::ShowError(int fd)
{
    dprintf(fd, "The number of recent errors recorded is %zu\n", g_errorInfo.size());
    int i = 0;
    for (const auto &it : g_errorInfo) {
        dprintf(fd, "Error ID: %d        ErrorInfo: %s\n", ++i, it.c_str());
    }
}

bool DumpHelper::Dump(int fd, const std::vector<std::string> &args)
{
    std::string command = "";
    std::string param = "";

    if (args.size() == CMD_NO_PARAM) {
        command = args.at(SUB_CMD_NAME);
    } else if (args.size() == CMD_HAS_PARAM) {
        command = args.at(SUB_CMD_NAME);
        param = args.at(SUB_CMD_PARAM);
    } else {
        ShowError(fd);
        if (!dumpAll_) {
            return false;
        }
        dumpAll_(fd);
    }

    if (command == CMD_HELP) {
        ShowHelp(fd);
    } else if (command == CMD_ERROR_INFO) {
        ShowError(fd);
    } else if (command == CMD_USER_INFO) {
        if (!dumpUserInfo_) {
            return false;
        }
        dumpUserInfo_(fd);
    } else if (command == CMD_APP_INFO) {
        if (!dumpAppInfo_) {
            return false;
        }
        dumpAppInfo_(fd, param);
    } else if (command == CMD_STORE_INFO) {
        if (!dumpStoreInfo_) {
            return false;
        }
        dumpStoreInfo_(fd, param);
    } else {
        ShowIllealInfomation(fd);
    }
    return true;
}

void DumpHelper::ShowHelp(int fd)
{
    std::string result;
    result.append("Usage:dump  <command> [options]\n")
        .append("Description:\n")
        .append(CMD_USER_INFO)
        .append("            ")
        .append("dump all user information in the system\n")
        .append(CMD_APP_INFO)
        .append("             ")
        .append("dump list of all app information in the system\n")
        .append(CMD_APP_INFO)
        .append(" [appID]     ")
        .append("dump information about the specified app in the system\n")
        .append(CMD_STORE_INFO)
        .append("           ")
        .append("dump list of all store information in the system\n")
        .append(CMD_STORE_INFO)
        .append(" [storeID] ")
        .append("dump information about the specified store in the system\n")
        .append(CMD_ERROR_INFO)
        .append("           ")
        .append("dump the recent errors information in the system\n");
    dprintf(fd, "%s\n", result.c_str());
}

void DumpHelper::ShowIllealInfomation(int fd)
{
    dprintf(fd, "%s\n", ILLEGAL_INFOMATION);
}
}  // namespace DistributedKv
}  // namespace OHOS


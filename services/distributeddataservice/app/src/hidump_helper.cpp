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

#include "hidump_helper.h"

#include <list>
#include "log_print.h"
#include "store_errno.h"
#include "kvstore_data_service.h"


namespace OHOS {
namespace DistributedKv {
namespace {
constexpr const int32_t SUPPORT_ARGS_SIZE = 1;
constexpr const int32_t MAX_ERROR = 10;
constexpr const int32_t FIRST_PARAM = 0;

constexpr const char *ARGS_HELP = "-h";
constexpr const char *ARGS_ALL = "-allInfo";
constexpr const char *ARGS_DEVICEID = "-device";
constexpr const char *ARGS_USER_INFO = "-userInfo";
constexpr const char *ARGS_APP_INFO = "-appInfo";
constexpr const char *ARGS_STORE_INFO = "-storeInfo";
constexpr const char *ARGS_ERROR_INFO = "-errorInfo";
constexpr const char *ILLEGAL_INFOMATION = "The arguments are illegal and you can enter '-h' for help.\n";

const std::map<std::string, HidumpFlag> ARGS_MAP = {
    { ARGS_HELP, HidumpFlag::GET_HELP },
    { ARGS_ALL, HidumpFlag::GET_ALL_INFO },
    { ARGS_DEVICEID, HidumpFlag::GET_DEVICE_INFO },
    { ARGS_USER_INFO, HidumpFlag::GET_USER_INFO },
    { ARGS_APP_INFO, HidumpFlag::GET_APP_INFO },
    { ARGS_STORE_INFO, HidumpFlag::GET_STORE_INFO },
    { ARGS_ERROR_INFO, HidumpFlag::GET_ERROR_INFO },
};
}

static std::list<std::string> g_errorInfo;
static int g_errorNum;

void HidumpHelper::AddErrorInfo(std::string &error)
{
    std::lock_guard<std::mutex> lock(hidumperMutex_);
    if (g_errorNum + 1 > MAX_ERROR) {
        g_errorNum = MAX_ERROR;
        g_errorInfo.pop_front();
        g_errorInfo.push_back(error);
    } else {
        g_errorNum++;
        g_errorInfo.push_back(error);
    }
}

void HidumpHelper::ShowError(int fd)
{
    dprintf(fd, "The number of recent errors recorded is %d\n", g_errorNum);
    if (!g_errorInfo.empty()) {
        std::list<std::string>::iterator it;
        int i = 0;
        for (it = g_errorInfo.begin(); it != g_errorInfo.end(); ++it) {
            dprintf(fd, "Error ID: %d        ErrorInfo: %s\n", ++i, (*it).c_str());
        }
    }
}

bool HidumpHelper::Dump(int fd, KvStoreDataService &kvStoreDataService, const std::vector<std::string> &args)
{
    Status errCode = SUCCESS;
    int32_t argsSize = static_cast<int32_t>(args.size());
    switch (argsSize) {
        case SUPPORT_ARGS_SIZE: {
            errCode = ProcessOneParam(fd, kvStoreDataService, args[FIRST_PARAM]);
            break;
        }
        default: {
            errCode = HIDUMP_INVAILD_ARGS;
            break;
        }
    }

    bool ret = false;
    switch (errCode) {
        case SUCCESS: {
            ret = true;
            break;
        }
        case HIDUMP_INVAILD_ARGS: {
            ShowIllealInfomation(fd);
            ret = true;
            break;
        }
        default: {
            break;
        }
    }

    return ret;
}

Status HidumpHelper::ProcessOneParam(int fd, KvStoreDataService &kvStoreDataService, const std::string &args)
{
    HidumpParam hidumpParam;
    auto operatorIter = ARGS_MAP.find(args);
    if (operatorIter != ARGS_MAP.end()) {
        hidumpParam.hidumpFlag = operatorIter->second;
    }

    if (operatorIter == ARGS_MAP.end()) {
        return HIDUMP_INVAILD_ARGS;
    }

    if (hidumpParam.hidumpFlag == HidumpFlag::GET_HELP) {
        ShowHelp(fd);
        return SUCCESS;
    }

    if (hidumpParam.hidumpFlag == HidumpFlag::GET_ERROR_INFO || hidumpParam.hidumpFlag == HidumpFlag::GET_ALL_INFO) {
        ShowError(fd);
    }

    return kvStoreDataService.DumpInner(fd, hidumpParam.hidumpFlag);
}

void HidumpHelper::ShowHelp(int fd)
{
    std::string result;
    result.append("Usage:dump  <command> [options]\n")
          .append("Description:\n")
          .append("-allInfo          ")
          .append("dump all information about distributedKvStore\n")
          .append("-device           ")
          .append("dump list of all deviceId in the system\n")
          .append("-userInfo         ")
          .append("dump all user information in the system\n")
          .append("-appInfo          ")
          .append("dump list of all app information in the system\n")
          .append("-storeInfo          ")
          .append("dump list of all store information in the system\n")
          .append("-errorInfo        ")
          .append("dump the recent errors information in the system\n");
    dprintf(fd, "%s\n", result.c_str());
}

void HidumpHelper::ShowIllealInfomation(int fd)
{
    dprintf(fd, "%s\n", ILLEGAL_INFOMATION);
}
}  // namespace DistributedKv
}  // namespace OHOS


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
#define LOG_TAG "DumpHelper"
#include "dump_helper.h"

#include <securec.h>

#include "log_print.h"
#include "rdb_types.h"
#include "runtime_config.h"
#include "string_ex.h"

namespace OHOS::DistributedData {
DumpHelper::DumpHelper()
{
    Handler handlerErrorInfo =
        std::bind(&DumpHelper::DumpErrorInfo, this, std::placeholders::_1, std::placeholders::_2);
    Handler handlerHelpInfo = std::bind(&DumpHelper::DumpHelpInfo, this, std::placeholders::_1, std::placeholders::_2);
    Handler handlerAllInfo = std::bind(&DumpHelper::DumpAllInfo, this, std::placeholders::_1, std::placeholders::_2);
    RegisterErrorInfo();
    DumpManager::GetInstance().AddHandler("ERROR_INFO", uintptr_t(this), handlerErrorInfo);
    RegisterHelpInfo();
    DumpManager::GetInstance().AddHandler("HELP_INFO", uintptr_t(this), handlerHelpInfo);
    RegisterAllInfo();
    DumpManager::GetInstance().AddHandler("ALL_INFO", uintptr_t(this), handlerAllInfo);
}

DumpHelper& DumpHelper::GetInstance()
{
    static DumpHelper instance;
    return instance;
}

bool DumpHelper::Dump(int fd, const std::vector<std::string> &args)
{
    std::vector<std::shared_ptr<CommandNode>> commands;
    ParseCommand(args, commands);
    GetCommandNodes(fd, commands);
    if (commands.empty()) {
        return true;
    }
    std::shared_ptr<CommandNode> tmpCommandNode;
    for (auto const& command : commands) {
        std::map<std::string, std::vector<std::string>> params;
        tmpCommandNode = command;
        while (tmpCommandNode != nullptr) {
            params.emplace(tmpCommandNode->dumpName, tmpCommandNode->param);
            if (tmpCommandNode->nextNode == nullptr) {
                for (auto const& handler : tmpCommandNode->handlers) {
                    handler(fd, params);
                }
            }
            tmpCommandNode = tmpCommandNode->nextNode;
        }
    }
    return true;
}

void DumpHelper::ParseCommand(const std::vector<std::string> &args, std::vector<std::shared_ptr<CommandNode>> &commands)
{
    std::shared_ptr<CommandNode> command;
    std::vector<std::string> tmpDumpNames;
    if (args.size() == 0) {
        command = std::make_shared<CommandNode>(GetCommand("ALL_INFO"));
        commands.emplace_back(command);
        return;
    }
    for (size_t i = 0; i < args.size(); i++) {
        std::vector<std::string> param;
        command = std::make_shared<CommandNode>(GetCommand(args[i]));
        if (!command->IsVoid()) {
            auto it = find(tmpDumpNames.begin(), tmpDumpNames.end(), command->dumpName);
            if (it != tmpDumpNames.end()) {
                ZLOGE("The same command is not allowed");
                return;
            }
            commands.emplace_back(command);
            tmpDumpNames.emplace_back(command->dumpName);
        } else {
            if (commands.empty()) {
                ZLOGE("Invalid Format");
                return;
            }
            commands.back()->param.emplace_back(args[i]);
            if (commands.back()->param.size() < commands.back()->minParamsNum ||
                commands.back()->param.size() > commands.back()->maxParamsNum) {
                ZLOGE("The number of param criteria is not legal");
                return;
            }
        }
    }
    if (commands.size() == 0) {
        ZLOGE("No command is entered or the command is invalid");
        return;
    }
}

DumpHelper::CommandNode DumpHelper::GetCommand(const std::string &name)
{
    auto config = DumpManager::GetInstance().GetConfig(name);
    CommandNode command;
    if (config.IsVoid()) {
        return command;
    }
    command.dumpName = config.dumpName;
    command.parentNode = config.parentNode;
    command.childNode = config.childNode;
    command.minParamsNum = config.minParamsNum;
    command.maxParamsNum = config.maxParamsNum;
    command.handlers = DumpManager::GetInstance().GetHandler(name);
    command.nextNode = nullptr;
    return command;
}

void DumpHelper::GetCommandNodes(int fd, std::vector<std::shared_ptr<CommandNode>> &commands)
{
    for (uint32_t i = 1; i < commands.size();) {
        bool isAdded = false;
        if (commands[i]->parentNode.empty()) {
            for (uint32_t j = 0; j < i; j++) {
                AddHeadNode(commands[i], commands[j], isAdded);
            }
        } else {
            for (uint32_t j = 0; j < i; j++) {
                AddNode(commands[i], commands[j], isAdded);
            }
        }
        if (!isAdded) {
            i++;
        } else {
            commands.erase(commands.begin() + i);
        }
    }
}

void DumpHelper::AddHeadNode(std::shared_ptr<CommandNode> &command, std::shared_ptr<CommandNode> &realHeadNode,
    bool &isAdded)
{
    auto tmpCommandNode = command;
    while (!(tmpCommandNode->childNode.empty())) {
        if (tmpCommandNode->childNode == realHeadNode->dumpName) {
            command->nextNode = realHeadNode;
            realHeadNode = command;
            isAdded = true;
        }
        tmpCommandNode = std::make_shared<CommandNode>(GetCommand(tmpCommandNode->childNode));
    }
}

void DumpHelper::AddNode(std::shared_ptr<CommandNode> &command, std::shared_ptr<CommandNode> &realHeadNode,
    bool &isAdded)
{
    auto tmpCommandNode = command;
    while (!(tmpCommandNode->parentNode.empty())) {
        auto tmpRealHeadNode = realHeadNode;
        while (tmpRealHeadNode != nullptr) {
            if (tmpCommandNode->parentNode == tmpRealHeadNode->dumpName) {
                command->nextNode = realHeadNode->nextNode;
                realHeadNode->nextNode = command;
                isAdded = true;
            }
            tmpRealHeadNode = tmpRealHeadNode->nextNode;
        }
        tmpCommandNode = std::make_shared<CommandNode>(GetCommand(tmpCommandNode->parentNode));
    }
}

void DumpHelper::RegisterErrorInfo()
{
    DumpManager::Config errorInfoConfig;
    errorInfoConfig.fullCmd = "--error-info";
    errorInfoConfig.abbrCmd = "-e";
    errorInfoConfig.dumpName = "ERROR_INFO";
    errorInfoConfig.dumpCaption = { "| Display the recent error messages" };
    DumpManager::GetInstance().AddConfig(errorInfoConfig.dumpName, errorInfoConfig);
}

void DumpHelper::DumpErrorInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    std::string info;
    for (const auto &it : errorInfo_) {
        std::string errorCode = std::to_string(it.errorCode);
        errorCode.resize(FORMAT_BLANK_SIZE, FORMAT_BLANK_SPACE);
        info.append(it.errorTime).append(errorCode).append(it.errorInfo).append("\n");
    }
    dprintf(fd,
        "-------------------------------------RecentError------------------------------------\nDate                   "
        "         ErrorCode                       ErrorMessage\n%s\n",
        info.c_str());
}

std::string DumpHelper::FormatHelpInfo(const std::string &cmd, const std::string &cmdAbbr, const std::string &paraExt,
    const std::string &info, uint32_t &formatMaxSize)
{
    std::string formatInfo;
    formatInfo.append(" ").append(cmdAbbr).append(paraExt).append(", ").append(cmd).append(paraExt);
    formatInfo.resize(formatMaxSize + FORMAT_FILL_SIZE, FORMAT_BLANK_SPACE);
    formatInfo.append(info);
    return formatInfo;
}

uint32_t DumpHelper::GetFormatMaxSize()
{
    uint32_t formatMaxSize = 0;
    auto dumpFactory = DumpManager::GetInstance().LoadConfig();
    dumpFactory.ForEach([&formatMaxSize](const auto &key, auto &value) {
        if (key == "ALL_INFO") {
            return false;
        }
        std::string helpInfo;
        helpInfo.append(" ").append(value.fullCmd).append(", ").append(value.abbrCmd);
        formatMaxSize = helpInfo.size() > formatMaxSize ? helpInfo.size() : formatMaxSize;
        std::string helpInfoAdd;
        if (value.countPrintf == PRINTF_COUNT_2) {
            helpInfoAdd.append(" ")
                .append(value.fullCmd)
                .append(", ")
                .append(value.abbrCmd)
                .append(value.infoName)
                .append(value.infoName);
            formatMaxSize = helpInfoAdd.size() > formatMaxSize ? helpInfoAdd.size() : formatMaxSize;
        }
        return false;
    });
    return formatMaxSize;
}

void DumpHelper::RegisterHelpInfo()
{
    DumpManager::Config helpInfoConfig;
    helpInfoConfig.fullCmd = "--help-info";
    helpInfoConfig.abbrCmd = "-h";
    helpInfoConfig.dumpName = "HELP_INFO";
    helpInfoConfig.dumpCaption = { "| Display this help message" };
    DumpManager::GetInstance().AddConfig(helpInfoConfig.dumpName, helpInfoConfig);
}

void DumpHelper::DumpHelpInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    std::string info;
    uint32_t formatMaxSize = GetFormatMaxSize();
    auto dumpFactory = DumpManager::GetInstance().LoadConfig();
    dumpFactory.ForEach([this, &info, &formatMaxSize](const auto &key, auto &value) {
        if (key == "ALL_INFO") {
            return false;
        }
        info.append(FormatHelpInfo(value.fullCmd, value.abbrCmd, "", value.dumpCaption[0], formatMaxSize)).append("\n");
        if (value.countPrintf == PRINTF_COUNT_2) {
            info.append(
                FormatHelpInfo(value.fullCmd, value.abbrCmd, value.infoName, value.dumpCaption[1], formatMaxSize))
                .append("\n");
        }
        return false;
    });
    dprintf(fd,
        "Usage: hidumper -s 1301 -a <option(s)>\nwhere possible options include:\n%s\nWhen -u/-u <UserId>, -b/-b "
        "<BundleName> or -s/-s <StoreID> is simultaneously selected,\nwe display the lowest level statistics where -u "
        "> -b > -s\nand the statistics is filterd by the upper level options\n",
        info.c_str());
}

void DumpHelper::RegisterAllInfo()
{
    DumpManager::Config allInfoConfig;
    allInfoConfig.fullCmd = "--all-info";
    allInfoConfig.abbrCmd = "-a";
    allInfoConfig.dumpName = "ALL_INFO";
    DumpManager::GetInstance().AddConfig(allInfoConfig.dumpName, allInfoConfig);
}

void DumpHelper::DumpAllInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    auto dumpFactory = DumpManager::GetInstance().LoadConfig();
    dumpFactory.ForEach([&fd, &params](const auto &key, auto &value) {
        if (key == "ALL_INFO" || key == "HELP_INFO") {
            return false;
        }
        std::vector<Handler> handlers = DumpManager::GetInstance().GetHandler(key);
        if (!(handlers.empty())) {
            for (auto handlerEach : handlers) {
                handlerEach(fd, params);
            }
        }
        return false;
    });
}

void DumpHelper::AddErrorInfo(int32_t errorCode, const std::string &errorInfo)
{
    ErrorInfo error;
    error.errorCode = errorCode;
    error.errorInfo = errorInfo;
    auto now = std::chrono::system_clock::now();
    int64_t millSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
                           (std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000);
    time_t tt = std::chrono::system_clock::to_time_t(now);
    auto ptm = localtime(&tt);
    if (ptm != nullptr) {
        char date[FORMAT_BLANK_SIZE] = { 0 };
        auto flag = sprintf_s(date, sizeof(date), "%04d-%02d-%02d  %02d:%02d:%02d %03d",
            (int)ptm->tm_year + DUMP_SYSTEM_START_YEAR, (int)ptm->tm_mon + 1, (int)ptm->tm_mday, (int)ptm->tm_hour,
            (int)ptm->tm_min, (int)ptm->tm_sec, (int)millSeconds);
        if (flag < 0) {
            ZLOGE("get date failed");
            return;
        }
        std::string errorTime = date;
        errorTime.resize(FORMAT_BLANK_SIZE, FORMAT_BLANK_SPACE);
        error.errorTime = errorTime;
    }
    std::lock_guard<std::mutex> lock(hidumperMutex_);
    if (errorInfo_.size() + 1 > MAX_RECORED_ERROR) {
        errorInfo_.pop_front();
        errorInfo_.push_back(error);
    } else {
        errorInfo_.push_back(error);
    }
}
} // namespace OHOS::DistributedData

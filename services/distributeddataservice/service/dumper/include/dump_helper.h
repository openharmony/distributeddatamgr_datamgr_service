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
#ifndef DISTRIBUTEDDATAMGR_SERVICE_DUMP_HELPER_H
#define DISTRIBUTEDDATAMGR_SERVICE_DUMP_HELPER_H

#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <set>

#include "concurrent_map.h"
#include "dump/dump_manager.h"
#include "metadata/store_meta_data.h"
#include "types.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class DumpHelper {
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;

public:
    struct CommandNode {
        std::string dumpName;
        std::vector<std::string> param;
        uint32_t minParamsNum = 0;
        uint32_t maxParamsNum = 0;
        std::string parentNode;
        std::string childNode;
        std::vector<Handler> handlers;
        std::shared_ptr<CommandNode> nextNode;
        bool IsVoid()
        {
            auto config = DumpManager::GetInstance().GetConfig(this->dumpName);
            return config.IsVoid();
        }
    };
    struct ErrorInfo {
        int32_t errorCode = 0;
        std::string errorTime;
        std::string errorInfo;
    };
    API_EXPORT static DumpHelper &GetInstance();
    API_EXPORT void AddErrorInfo(int32_t errorCode, const std::string &errorInfo);
    API_EXPORT bool Dump(int fd, const std::vector<std::string> &args);

private:
    DumpHelper();
    ~DumpHelper() = default;
    void GetCommandNodes(int fd, std::vector<std::shared_ptr<CommandNode>> &commands);
    CommandNode GetCommand(const std::string &name);
    void AddHeadNode(std::shared_ptr<CommandNode> &command, std::shared_ptr<CommandNode> &realHeadNode, bool &isAdded);
    void AddNode(std::shared_ptr<CommandNode> &command, std::shared_ptr<CommandNode> &realHeadNode, bool &isAdded);
    void ParseCommand(const std::vector<std::string> &args, std::vector<std::shared_ptr<CommandNode>> &filterInfo);
    uint32_t GetFormatMaxSize();

    void RegisterErrorInfo();
    void DumpErrorInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    void RegisterHelpInfo();
    void DumpHelpInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    void RegisterAllInfo();
    void DumpAllInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    std::string FormatHelpInfo(const std::string &cmdAbbr, const std::string &cmd, const std::string &paraExt,
        const std::string &info, uint32_t &formatMaxSize);
    mutable std::mutex hidumperMutex_;
    static constexpr int32_t MAX_FILTER_COUNT = 3;
    static constexpr int32_t MAX_RECORED_ERROR = 10;
    static constexpr int32_t DUMP_SYSTEM_START_YEAR = 1900;
    static constexpr int32_t FORMAT_BLANK_SIZE = 32;
    static constexpr int32_t FORMAT_FILL_SIZE = 6;
    static constexpr char FORMAT_BLANK_SPACE = ' ';
    static constexpr int32_t DUMP_COMMAND_PREFIX_SIZE = 1;
    static constexpr const char *DUMP_COMMAND_PREFIX = "-";
    static constexpr const char *INDENTATION = "    ";
    static constexpr const char *SEPARATOR = "_";
    static constexpr int32_t PRINTF_COUNT_2 = 2;

    std::list<ErrorInfo> errorInfo_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_SERVICE_DUMP_HELPER_H

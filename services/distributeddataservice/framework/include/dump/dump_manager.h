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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_DUMPMANAGER_DUMP_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_DUMPMANAGER_DUMP_MANAGER_H
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "concurrent_map.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class DumpManager {
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;

public:
    struct Config {
        std::string dumpName;
        std::string fullCmd;
        std::string abbrCmd;
        int countPrintf = 1;
        std::string infoName;
        uint32_t minParamsNum = 0;
        uint32_t maxParamsNum = 0;
        std::string parentNode;
        std::string childNode;
        std::vector<std::string> dumpCaption;
        bool IsVoid()
        {
            if (fullCmd.empty()) {
                return true;
            }
            return false;
        }
    };
    API_EXPORT static DumpManager &GetInstance();
    API_EXPORT void AddConfig(const std::string &name, Config &config);
    API_EXPORT Config GetConfig(const std::string &name);
    API_EXPORT ConcurrentMap<std::string, DumpManager::Config> LoadConfig();
    API_EXPORT void AddHandler(const std::string &infoName, uintptr_t ptr, const Handler &handler);
    API_EXPORT std::vector<Handler> GetHandler(const std::string &infoName);
    API_EXPORT void RemoveHandler(const std::string &infoName, uintptr_t ptr);

private:
    ConcurrentMap<std::string, Config> factory_;
    ConcurrentMap<std::string, std::string> indexTable_;
    ConcurrentMap<std::string, std::map<uintptr_t, Handler>> handlers_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_DUMPMANAGER_DUMP_MANAGER_H

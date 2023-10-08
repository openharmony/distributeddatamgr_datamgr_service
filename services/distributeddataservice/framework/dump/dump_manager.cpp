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
#include "dump/dump_manager.h"
namespace OHOS {
namespace DistributedData {
DumpManager &DumpManager::GetInstance()
{
    static DumpManager instance;
    return instance;
}

void DumpManager::AddConfig(const std::string &name, Config &config)
{
    factory_.Insert(name, config);
    indexTable_.Insert(config.fullCmd, name);
    indexTable_.Insert(config.abbrCmd, name);
}

DumpManager::Config DumpManager::GetConfig(const std::string &name)
{
    auto it = factory_.Find(name);
    if (it.first) {
        return (it.second);
    }
    auto itIndex = indexTable_.Find(name);
    if (itIndex.first) {
        return GetConfig(itIndex.second);
    }
    return {};
}

ConcurrentMap<std::string, DumpManager::Config> DumpManager::LoadConfig()
{
    return factory_;
}

void DumpManager::AddHandler(const std::string &infoName, uintptr_t ptr, const Handler &handler)
{
    handlers_.Compute(infoName, [ptr, &handler](const auto &key, auto &value) {
        value.emplace(ptr, handler);
        return true;
    });
}

std::vector<DumpManager::Handler> DumpManager::GetHandler(const std::string &infoName)
{
    auto it = handlers_.Find(infoName);
    std::vector<Handler> handlers;
    if (it.first) {
        for (auto &handler : it.second) {
            handlers.emplace_back(handler.second);
        }
        return handlers;
    }
    auto itIndex = indexTable_.Find(infoName);
    if (itIndex.first) {
        return GetHandler(itIndex.second);
    }
    return handlers;
}

void DumpManager::RemoveHandler(const std::string &infoName, uintptr_t ptr)
{
    handlers_.ComputeIfPresent(infoName, [ptr](const auto &key, auto &value) {
        value.erase(ptr);
        return !value.empty();
    });
}
} // namespace DistributedData
} // namespace OHOS
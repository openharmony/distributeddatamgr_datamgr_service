/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "config_factory.h"
#include <fstream>
namespace OHOS {
namespace DistributedData {
ConfigFactory::ConfigFactory()
    : file_(std::string(CONF_PATH) + "/config.json")
{
}

ConfigFactory::~ConfigFactory()
{
}

ConfigFactory &ConfigFactory::GetInstance()
{
    static ConfigFactory factory;
    if (!factory.isInited) {
        factory.Initialize();
    }
    return factory;
}

int32_t ConfigFactory::Initialize()
{
    std::string jsonStr;
    std::ifstream fin(file_);
    while (fin.good()) {
        std::string line;
        std::getline(fin, line);
        jsonStr += line;
    }
    config_.Unmarshall(jsonStr);
    isInited = true;
    return 0;
}

std::vector<ComponentConfig> *ConfigFactory::GetComponentConfig()
{
    return config_.components;
}

NetworkConfig *ConfigFactory::GetNetworkConfig()
{
    return config_.networks;
}

CheckerConfig *ConfigFactory::GetCheckerConfig()
{
    return config_.bundleChecker;
}

GlobalConfig *ConfigFactory::GetGlobalConfig()
{
    return &config_;
}

DirectoryConfig *ConfigFactory::GetDirectoryConfig()
{
    return config_.directory;
}

BackupConfig *ConfigFactory::GetBackupConfig()
{
    return config_.backup;
}
} // namespace DistributedData
} // namespace OHOS

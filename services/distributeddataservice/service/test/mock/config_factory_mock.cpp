/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
ConfigFactory::ConfigFactory() : file_(std::string(CONF_PATH) + "/config.json")
{
}

ConfigFactory::~ConfigFactory()
{
}

ConfigFactory &ConfigFactory::GetInstance()
{
    static ConfigFactory factory;
    return factory;
}

int32_t ConfigFactory::Initialize()
{
    return 0;
}

std::vector<ComponentConfig> *ConfigFactory::GetComponentConfig()
{
    return nullptr;
}

NetworkConfig *ConfigFactory::GetNetworkConfig()
{
    return nullptr;
}

CheckerConfig *ConfigFactory::GetCheckerConfig()
{
    return nullptr;
}

GlobalConfig *ConfigFactory::GetGlobalConfig()
{
    return nullptr;
}

DirectoryConfig *ConfigFactory::GetDirectoryConfig()
{
    return nullptr;
}

BackupConfig *ConfigFactory::GetBackupConfig()
{
    return nullptr;
}

CloudConfig *ConfigFactory::GetCloudConfig()
{
    return nullptr;
}

std::vector<AppIdMappingConfig> *ConfigFactory::GetAppIdMappingConfig()
{
    return nullptr;
}

ThreadConfig *ConfigFactory::GetThreadConfig()
{
    return nullptr;
}

DataShareConfig *ConfigFactory::GetDataShareConfig()
{
    return nullptr;
}

std::vector<DoubleSyncSAConfig> *ConfigFactory::GetDoubleSyncSAConfig()
{
    return nullptr;
}
} // namespace DistributedData
} // namespace OHOS
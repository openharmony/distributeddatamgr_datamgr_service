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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_CONFIG_FACTORY_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_CONFIG_FACTORY_H
#include <cstdint>
#include "visibility.h"
#include "model/global_config.h"
namespace OHOS {
namespace DistributedData {
class ConfigFactory {
public:
    API_EXPORT static ConfigFactory &GetInstance();
    API_EXPORT int32_t Initialize();
    API_EXPORT std::vector<ComponentConfig> *GetComponentConfig();
    API_EXPORT NetworkConfig *GetNetworkConfig();
    API_EXPORT CheckerConfig *GetCheckerConfig();
    API_EXPORT GlobalConfig *GetGlobalConfig();
    API_EXPORT DirectoryConfig *GetDirectoryConfig();
    API_EXPORT BackupConfig *GetBackupConfig();
private:
    static constexpr const char *CONF_PATH = "/system/etc/distributeddata/conf";
    ConfigFactory();
    ~ConfigFactory();

    std::string file_;
    GlobalConfig config_;
    bool isInited = false;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_CONFIG_FACTORY_H

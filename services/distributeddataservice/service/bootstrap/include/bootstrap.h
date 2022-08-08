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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_BOOTSTRAP_BOOTSTRAP_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_BOOTSTRAP_BOOTSTRAP_H
#include <string>
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class Bootstrap {
public:
    API_EXPORT static Bootstrap &GetInstance();
    API_EXPORT std::string GetProcessLabel();
    API_EXPORT std::string GetMetaDBName();
    API_EXPORT void LoadComponents();
    API_EXPORT void LoadCheckers();
    API_EXPORT void LoadNetworks();
    API_EXPORT void LoadDirectory();
    API_EXPORT void LoadBackup();
private:
    static constexpr const char *DEFAULT_LABEL = "distributeddata";
    static constexpr const char *DEFAULT_META = "service_meta";
    using Constructor = void(*)(const char *);
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_BOOTSTRAP_BOOTSTRAP_H

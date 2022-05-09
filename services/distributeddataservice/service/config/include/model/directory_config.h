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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_MODEL_DIRECTORY_CONFIG_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_MODEL_DIRECTORY_CONFIG_H
#include "directory_manager.h"
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
class DirectoryConfig final : public Serializable {
public:
    struct DirectoryStrategy final : public Serializable, public DirectoryManager::Strategy {
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };
    std::vector<DirectoryStrategy> strategy;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CONFIG_MODEL_DIRECTORY_CONFIG_H

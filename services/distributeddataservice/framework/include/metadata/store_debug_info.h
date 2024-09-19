/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_DEBUG_INFO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_DEBUG_INFO_H
#include "serializable/serializable.h"
#include <map>
namespace OHOS::DistributedData {
struct API_EXPORT StoreDebugInfo final : public Serializable {
    struct FileInfo final : public Serializable {
        uint64_t inode = 0;
        uint64_t size = 0;
        uint32_t dev = 0;
        uint32_t mode = 0;
        uint32_t uid = 0;
        uint32_t gid = 0;
        bool Marshal(Serializable::json &node) const override;
        bool Unmarshal(const Serializable::json &node) override;

    };
    std::map<std::string, FileInfo> fileInfos;
    API_EXPORT static std::string GetPrefix(const std::initializer_list<std::string> &fields);
    bool Marshal(Serializable::json &node) const override;
    bool Unmarshal(const Serializable::json &node) override;
private:
    static constexpr const char *KEY_PREFIX = "StoreDebugInfo";
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_DEBUG_INFO_H

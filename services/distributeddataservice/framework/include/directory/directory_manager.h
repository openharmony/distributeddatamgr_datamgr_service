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

#ifndef DISTRIBUTEDDATAMGR_DIRECTORY_MANAGER_H
#define DISTRIBUTEDDATAMGR_DIRECTORY_MANAGER_H

#include <map>
#include <memory>

#include "metadata/store_meta_data.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class DirectoryManager {
public:
    static constexpr uint32_t INVALID_VERSION = 0xFFFFFFFF;
    struct Strategy {
        bool autoCreate = false;
        uint32_t version = 0;
        std::string pattern;
        std::string metaPath;
    };
    API_EXPORT static DirectoryManager &GetInstance();
    API_EXPORT std::string GetStorePath(const StoreMetaData &metaData, uint32_t version = INVALID_VERSION);
    API_EXPORT std::string GetSecretKeyPath(const StoreMetaData &metaData, uint32_t version = INVALID_VERSION);
    API_EXPORT std::string GetStoreBackupPath(const StoreMetaData &metaData, uint32_t version = INVALID_VERSION);
    API_EXPORT std::string GetMetaStorePath(uint32_t version = INVALID_VERSION);
    API_EXPORT std::string GetMetaBackupPath(uint32_t version = INVALID_VERSION);
    API_EXPORT std::vector<uint32_t> GetVersions();
    API_EXPORT void Initialize(const std::vector<Strategy> &strategies);

private:
    using Action = std::string (DirectoryManager::*)(const StoreMetaData &) const;
    struct StrategyImpl {
        bool autoCreate = false;
        uint32_t version;
        std::string metaPath;
        std::vector<std::string> path;
        std::vector<Action> pipes;
    };

    DirectoryManager();
    std::string GetType(const StoreMetaData &metaData) const;
    std::string GetStore(const StoreMetaData &metaData) const;
    std::string GetSecurity(const StoreMetaData &metaData) const;
    std::string GetArea(const StoreMetaData &metaData) const;
    std::string GetUserId(const StoreMetaData &metaData) const;
    std::string GetBundleName(const StoreMetaData &metaData) const;
    std::string GetHapName(const StoreMetaData &metaData) const;
    std::vector<std::string> Split(const std::string &source, const std::string &pattern) const;
    int32_t GetVersionIndex(uint32_t version) const;
    std::string GenPath(const StoreMetaData &metaData, uint32_t version, const std::string &exPath = "") const;
    bool CreateDirectory(const std::string &path) const;
    const std::map<std::string, Action> actions_;
    std::vector<StrategyImpl> strategies_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_DIRECTORY_MANAGER_H

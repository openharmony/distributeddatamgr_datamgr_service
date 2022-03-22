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
#include "visibility.h"
#include "kvstore_context.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedData {
enum PathType {
    PATH_DE,
    PATH_CE,
};
class DirWorker {
public:
    DirWorker() = default;
    virtual std::string GetDir(ClientContext clientContext, PathType type) = 0;
    virtual bool CreateDir(ClientContext clientContext, PathType type) = 0;
    virtual std::string GetBackupDir(ClientContext clientContext, PathType type) = 0;
    virtual std::string GetSecretKeyDir(ClientContext clientContext, PathType type) = 0;
    virtual std::string GetMetaDir() = 0;
};

class ServerDirWorker : public DirWorker {
public:
    static ServerDirWorker &GetInstance();

    std::string GetDir(ClientContext clientContext, PathType type) override;
    bool CreateDir(ClientContext clientContext, PathType type) override;
    std::string GetBackupDir(ClientContext clientContext, PathType type) override;
    std::string GetSecretKeyDir(ClientContext clientContext, PathType type) override;
    std::string GetMetaDir() override;

public:
    static std::map<PathType, std::string> rootPathMap_;
};

class ClientDirWorker : public DirWorker {
public:
    static ClientDirWorker &GetInstance();

    std::string GetDir(ClientContext clientContext, PathType type) override;
    bool CreateDir(ClientContext clientContext, PathType type) override;
    std::string GetBackupDir(ClientContext clientContext, PathType type) override;
    std::string GetSecretKeyDir(ClientContext clientContext, PathType type) override;
    std::string GetMetaDir() override;
};

class DirectoryManager {
public:
    struct Strategy {
        std::string version;
        std::string holder;
        std::string path;
        std::string metaPath;
    };
    API_EXPORT static DirectoryManager &GetInstance();
    API_EXPORT std::string CreatePath(const ClientContext &context, PathType type);
    API_EXPORT std::string GetStorePath(const StoreMetaData &metaData);
    API_EXPORT std::string GetStoreBackupPath(const StoreMetaData &metaData);
    API_EXPORT std::string GetMetaDataStorePath();

    API_EXPORT void AddParams(const Strategy &strategy);
    API_EXPORT void SetCurrentVersion(const std::string &version);

    inline static std::string JoinPath(std::initializer_list<std::string> stringList)
    {
        std::string tmpPath;
        for (const std::string &str : stringList) {
            tmpPath += (str + "/");
        }
        return tmpPath;
    }

private:
    std::map<std::string, Strategy> patterns_;
    std::string version_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_DIRECTORY_MANAGER_H

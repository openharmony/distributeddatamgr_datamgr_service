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
#define LOG_TAG "DirectoryManager"
#include "directory_manager.h"

#include <string>

#include "utils/constant.h"
#include "directory_ex.h"
#include "kvstore_context.h"
#include "log/log_print.h"
namespace OHOS::DistributedData {
using namespace DistributedKv;
std::map<PathType, std::string> ServerDirWorker::rootPathMap_ = {
    { PATH_DE, "/data/misc_de/0/mdds" },
    { PATH_CE, "/data/misc_ce/0/mdds" },
};
std::string ServerDirWorker::GetDir(ClientContext clientContext, PathType type)
{
    if (rootPathMap_.find(type) == rootPathMap_.end()) {
        return "";
    }
    return DirectoryManager::JoinPath({ rootPathMap_.at(type), clientContext.userId,
        Constant::GetDefaultHarmonyAccountName(), clientContext.bundleName });
}
bool ServerDirWorker::CreateDir(ClientContext clientContext, PathType type)
{
    std::string directory = GetDir(clientContext, type);
    bool ret = ForceCreateDirectory(directory);
    if (!ret) {
        ZLOGE("create directory[%s] failed, errstr=[%d].", directory.c_str(), errno);
        return false;
    }
    return true;
}
ServerDirWorker &ServerDirWorker::GetInstance()
{
    static ServerDirWorker instance;
    return instance;
}
std::string ServerDirWorker::GetBackupDir(ClientContext clientContext, PathType type)
{
    if (rootPathMap_.find(type) == rootPathMap_.end()) {
        return "";
    }
    return DirectoryManager::JoinPath({ rootPathMap_.at(type), clientContext.userId,
        Constant::GetDefaultHarmonyAccountName(), clientContext.bundleName, "backup" });
}
std::string ServerDirWorker::GetMetaDir()
{
    return DirectoryManager::JoinPath({ rootPathMap_.at(PATH_DE), "Meta" });
}
std::string ServerDirWorker::GetSecretKeyDir(ClientContext clientContext, PathType type)
{
    return GetDir(clientContext, type);
}
std::string ClientDirWorker::GetDir(ClientContext clientContext, PathType type)
{
    return clientContext.dataDir;
}
bool ClientDirWorker::CreateDir(ClientContext clientContext, PathType type)
{
    return true;
}
ClientDirWorker &ClientDirWorker::GetInstance()
{
    static ClientDirWorker instance;
    return instance;
}
std::string ClientDirWorker::GetBackupDir(ClientContext clientContext, PathType type)
{
    return DirectoryManager::JoinPath({ clientContext.dataDir, "backup" });
}
std::string ClientDirWorker::GetMetaDir()
{
    return std::string("/data/service/el1/public/distributeddata/DistributedKvDataService/Meta/");
}
std::string ClientDirWorker::GetSecretKeyDir(ClientContext clientContext, PathType type)
{
    return GetDir(clientContext, type);
}

std::string DirectoryManager::CreatePath(const ClientContext &context, PathType type)
{
    return "";
}

DirectoryManager &DirectoryManager::GetInstance()
{
    static DirectoryManager instance;
    return instance;
}

std::string DirectoryManager::GetStorePath(const StoreMetaData &metaData)
{
    return metaData.dataDir;
}

std::string DirectoryManager::GetStoreBackupPath(const StoreMetaData &metaData)
{
    return GetStorePath(metaData) + "/backup/";
}

std::string DirectoryManager::GetMetaDataStorePath()
{
    return "/data/service/el1/public/distributeddata/meta/";
}
void DirectoryManager::AddParams(const Strategy &strategy)
{
    patterns_[strategy.version] = strategy;
}
void DirectoryManager::SetCurrentVersion(const std::string &version)
{
    version_ = version;
}
} // namespace OHOS::DistributedData

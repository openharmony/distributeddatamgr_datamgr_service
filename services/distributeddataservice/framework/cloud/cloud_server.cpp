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

#include "cloud/cloud_server.h"
namespace OHOS::DistributedData {
CloudServer *CloudServer::instance_ = nullptr;
CloudServer *CloudServer::GetInstance()
{
    return instance_;
}

bool CloudServer::RegisterCloudInstance(CloudServer *instance)
{
    if (instance_ != nullptr) {
        return false;
    }
    instance_ = instance;
    return true;
}

CloudInfo CloudServer::GetServerInfo(int32_t userId)
{
    return CloudInfo();
}

SchemaMeta CloudServer::GetAppSchema(int32_t userId, const std::string &bundleName)
{
    return SchemaMeta();
}

std::shared_ptr<AssetLoader> CloudServer::ConnectAssetLoader(uint32_t tokenId, const CloudServer::Database &dbMeta)
{
    return nullptr;
}

std::shared_ptr<CloudDB> CloudServer::ConnectCloudDB(uint32_t tokenId, const CloudServer::Database &dbMeta)
{
    return nullptr;
}
} // namespace OHOS::DistributedData
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_MOCK_CLOUD_SERVER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_MOCK_CLOUD_SERVER_H
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_info.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "cloud/sharing_center.h"
#include "executor_pool.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class MockCloudServer : public CloudServer {
public:
    std::pair<int32_t, CloudInfo> GetServerInfo(int32_t userId, bool needSpaceInfo = true) override
    {
        return { 0, CloudInfo() };
    }
    std::pair<int32_t, SchemaMeta> GetAppSchema(int32_t userId, const std::string &bundleName) override
    {
        return { 0, SchemaMeta() };
    }
    int32_t Subscribe(int32_t userId, const std::map<std::string, std::vector<Database>> &dbs) override
    {
        return 0;
    }
    int32_t Unsubscribe(int32_t userId, const std::map<std::string, std::vector<Database>> &dbs) override
    {
        return 0;
    }
    std::shared_ptr<AssetLoader> ConnectAssetLoader(
        const std::string &bundleName, int user, const Database &dbMeta) override
    {
        return nullptr;
    }
    std::shared_ptr<AssetLoader> ConnectAssetLoader(uint32_t tokenId, const Database &dbMeta) override
    {
        return nullptr;
    }
    std::shared_ptr<CloudDB> ConnectCloudDB(const std::string &bundleName, int user, const Database &dbMeta) override
    {
        return nullptr;
    }
    std::shared_ptr<CloudDB> ConnectCloudDB(uint32_t tokenId, const Database &dbMeta) override
    {
        return nullptr;
    }
    std::shared_ptr<SharingCenter> ConnectSharingCenter(int32_t userId, const std::string &bunleName) override
    {
        return nullptr;
    }
    void Clean(int32_t userId) override
    {
    }
    void ReleaseUserInfo(int32_t userId) override
    {
    }
    void Bind(std::shared_ptr<ExecutorPool> executor) override
    {
    }
    bool IsSupportCloud(int32_t userId) override
    {
        return false;
    }
    bool CloudDriverUpdated(const std::string &bundleName) override
    {
        return false;
    }
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_MOCK_CLOUD_SERVER_H
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_ASSET_LOADER_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_ASSET_LOADER_H

#include "cloud/asset_loader.h"
#include "cloud/cloud_store_types.h"
#include "cloud/iAssetLoader.h"
#include "error/general_error.h"
#include "snapshot/snapshot.h"

namespace OHOS::DistributedRdb {
class RdbAssetLoader : public DistributedDB::IAssetLoader {
public:
    using Type = DistributedDB::Type;
    using Asset = DistributedDB::Asset;
    using DBStatus = DistributedDB::DBStatus;
    using BindAssets = DistributedData::BindAssets;
    using AssetsRecord = DistributedData::AssetRecord;
    using AssetStatus = DistributedData::Asset::Status;
    using GeneralError = DistributedData::GeneralError;
    using VBucket = DistributedData::VBucket;

    explicit RdbAssetLoader(std::shared_ptr<DistributedData::AssetLoader> cloudAssetLoader, BindAssets *bindAssets);

    ~RdbAssetLoader() = default;

    DBStatus Download(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, std::vector<Asset>> &assets) override;

    void BatchDownload(const std::string &tableName, std::vector<AssetRecord> &downloadAssets) override;

    DBStatus RemoveLocalAssets(const std::vector<Asset> &assets) override;

    DBStatus CancelDownload() override;

private:
    static std::vector<AssetRecord> Convert(std::vector<AssetsRecord> &&assetsRecords);
    static std::vector<AssetsRecord> Convert(std::vector<AssetRecord> &&assetRecords);
    static DBStatus ConvertStatus(AssetStatus error);
    static void UpdateStatus(AssetRecord &assetRecord, const VBucket &assets);

    void PostEvent(std::set<std::string> &skipAssets, std::map<std::string, DistributedData::Value> &assets,
        DistributedData::AssetEvent eventId, std::set<std::string> &deleteAssets);
    void PostEvent(std::set<std::string> &skipAssets, std::vector<AssetsRecord> &assetsRecords,
        DistributedData::AssetEvent eventId, std::set<std::string> &deleteAssets);
    void PostEvent(DistributedData::AssetEvent eventId, DistributedData::Assets &assets,
        std::set<std::string> &skipAssets, std::set<std::string> &deleteAssets);

    std::shared_ptr<DistributedData::AssetLoader> assetLoader_;
    BindAssets *snapshots_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_ASSET_LOADER_H
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

namespace OHOS::DistributedRdb {
class RdbAssetLoader : public DistributedDB::IAssetLoader {
public:
    using Type = DistributedDB::Type;
    using Asset = DistributedDB::Asset;
    using DBStatus = DistributedDB::DBStatus;
    explicit RdbAssetLoader(std::shared_ptr<DistributedData::AssetLoader> cloudAssetLoader);

    ~RdbAssetLoader() = default;

    DBStatus Download(const std::string &tableName, const std::string &gid, const Type &prefix,
        std::map<std::string, std::vector<Asset>> &assets) override;

private:
    DBStatus ConvertStatus(DistributedData::GeneralError error);

    std::shared_ptr<DistributedData::AssetLoader> assetLoader_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_ASSET_LOADER_H
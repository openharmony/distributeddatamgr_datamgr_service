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

#define LOG_TAG "RdbAssetLoader"
#include "rdb_asset_loader.h"

#include "error/general_error.h"
#include "log_print.h"
#include "rdb_cloud.h"
#include "value_proxy.h"

using namespace DistributedDB;
namespace OHOS::DistributedRdb {
RdbAssetLoader::RdbAssetLoader(std::shared_ptr<DistributedData::AssetLoader> cloudAssetLoader)
    : assetLoader_(std::move(cloudAssetLoader))
{
}

DBStatus RdbAssetLoader::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    DistributedData::VBucket downLoadAssets = ValueProxy::Convert(assets);
    DistributedDB::Type prefixTemp = prefix;
    auto error = assetLoader_->Download(tableName, gid, ValueProxy::Convert(std::move(prefixTemp)), downLoadAssets);
    if (error == DistributedData::GeneralError::E_OK) {
        assets = ValueProxy::Convert(std::move(downLoadAssets));
    }
    return RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}

DBStatus RdbAssetLoader::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    DistributedData::VBucket deleteAssets = ValueProxy::Convert(std::map<std::string, Assets>{{ "", assets }});
    auto error = assetLoader_->RemoveLocalAssets("", "", {}, deleteAssets);
    return RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}
} // namespace OHOS::DistributedRdb
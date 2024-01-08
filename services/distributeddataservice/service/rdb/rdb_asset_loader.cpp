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
using ValueProxy = OHOS::DistributedData::ValueProxy;
namespace OHOS::DistributedRdb {
RdbAssetLoader::RdbAssetLoader(std::shared_ptr<DistributedData::AssetLoader> cloudAssetLoader, BindAssets* bindAssets)
    : assetLoader_(std::move(cloudAssetLoader)), snapshots_(bindAssets)
{
}

DBStatus RdbAssetLoader::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    DistributedData::VBucket downLoadAssets = ValueProxy::Convert(assets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    PostEvent(skipAssets, downLoadAssets, DistributedData::AssetEvent::DOWNLOAD, deleteAssets);
    DistributedDB::Type prefixTemp = prefix;
    auto error = assetLoader_->Download(tableName, gid, ValueProxy::Convert(std::move(prefixTemp)), downLoadAssets);
    PostEvent(skipAssets, downLoadAssets, DistributedData::AssetEvent::DOWNLOAD_FINISHED, deleteAssets);
    assets = ValueProxy::Convert(std::move(downLoadAssets));
    return skipAssets.empty() ? RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error))
                              : CLOUD_RECORD_EXIST_CONFLICT;
}

DBStatus RdbAssetLoader::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    DistributedData::VBucket deleteAssets = ValueProxy::Convert(std::map<std::string, Assets>{{ "", assets }});
    auto error = assetLoader_->RemoveLocalAssets("", "", {}, deleteAssets);
    return RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}

void RdbAssetLoader::PostEvent(std::set<std::string>& skipAssets, std::map<std::string, DistributedData::Value>& assets,
    DistributedData::AssetEvent eventId, std::set<std::string>& deleteAssets)
{
    for (auto& asset : assets) {
        auto* downLoadAssets = Traits::get_if<DistributedData::Assets>(&asset.second);
        if (downLoadAssets == nullptr) {
            return;
        }
        PostEvent(eventId, *downLoadAssets, skipAssets, deleteAssets);
    }
}

void RdbAssetLoader::PostEvent(DistributedData::AssetEvent eventId, DistributedData::Assets& assets,
    std::set<std::string>& skipAssets, std::set<std::string>& deleteAssets)
{
    for (auto& downLoadAsset : assets) {
        if (downLoadAsset.status == DistributedData::Asset::STATUS_DELETE) {
            deleteAssets.insert(downLoadAsset.uri);
            continue;
        }
        if (snapshots_->bindAssets == nullptr) {
            continue;
        }
        auto it = snapshots_->bindAssets->find(downLoadAsset.uri);
        if (it == snapshots_->bindAssets->end() || it->second == nullptr) {
            continue;
        }
        auto snapshot = it->second;
        if (eventId == DistributedData::DOWNLOAD) {
            snapshot->Download(downLoadAsset);
            if (snapshot->GetAssetStatus(downLoadAsset) == DistributedData::STATUS_WAIT_DOWNLOAD) {
                skipAssets.insert(downLoadAsset.uri);
            }
        } else {
            auto skipPos = skipAssets.find(downLoadAsset.uri);
            auto deletePos = deleteAssets.find(downLoadAsset.uri);
            if (skipPos != skipAssets.end() || deletePos != skipAssets.end()) {
                continue;
            }
            snapshot->Downloaded(downLoadAsset);
        }
    }
}
} // namespace OHOS::DistributedRdb
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
RdbAssetLoader::RdbAssetLoader(std::shared_ptr<DistributedData::AssetLoader> cloudAssetLoader, Snapshots* snapshots)
    : assetLoader_(std::move(cloudAssetLoader)), snapshots_(snapshots)
{
}

DBStatus RdbAssetLoader::Download(const std::string &tableName, const std::string &gid, const Type &prefix,
    std::map<std::string, Assets> &assets)
{
    DistributedData::VBucket downLoadAssets = ValueProxy::Convert(assets);
    std::set<std::string> skipAssets;
    bool needCompensate = false;
    StartDownloadingInSnapshot(downLoadAssets, skipAssets, needCompensate);
    DistributedDB::Type prefixTemp = prefix;
    auto error = assetLoader_->Download(tableName, gid, ValueProxy::Convert(std::move(prefixTemp)), downLoadAssets);
    FinishDownloadingInSnapshot(downLoadAssets, skipAssets);
    assets = ValueProxy::Convert(std::move(downLoadAssets));
    return needCompensate ? CLOUD_RECORD_EXIST_CONFLICT : RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}

void RdbAssetLoader::StartDownloadingInSnapshot(std::map<std::string, DistributedData::Value>& assets,
    std::set<std::string>& skipAssets, bool& needCompensate)
{
    for (auto& asset : assets) {
        if (asset.second.index() == DistributedData::TYPE_INDEX<DistributedData::Assets>) {
            auto* downLoadAssets = Traits::get_if<DistributedData::Assets>(&asset.second);
            if (downLoadAssets == nullptr) {
                return;
            }
            for (auto& downLoadAsset : *downLoadAssets) {
                auto it = snapshots_->snapshots->find(downLoadAsset.uri);
                if (downLoadAsset.status == DistributedData::Asset::STATUS_DELETE) {
                    continue;
                }
                if (it == snapshots_->snapshots->end()) {
                    continue;
                }

                auto snapShot = snapshots_->snapshots->at(downLoadAsset.uri);
                snapShot->Download(downLoadAsset);
                if (snapShot->GetAssetStatus(downLoadAsset) == DistributedData::STATUS_WAIT_DOWNLOAD) {
                    needCompensate = true;
                    skipAssets.insert(downLoadAsset.uri);
                }
            }
        }
    }
}

void RdbAssetLoader::FinishDownloadingInSnapshot(std::map<std::string, DistributedData::Value>& assets,
    std::set<std::string>& skipAssets)
{
    for (auto& asset : assets) {
        if (asset.second.index() == DistributedData::TYPE_INDEX<DistributedData::Assets>) {
            auto* downLoadAssets = Traits::get_if<DistributedData::Assets>(&asset.second);
            if (downLoadAssets == nullptr) {
                return;
            }
            for (auto& downLoadAsset : *downLoadAssets) {
                auto it = snapshots_->snapshots->find(downLoadAsset.uri);
                if (it == snapshots_->snapshots->end()) {
                    continue;
                }
                auto pos = skipAssets.find(downLoadAsset.uri);
                if (pos != skipAssets.end()) {
                    continue;
                }
                snapshots_->snapshots->at(downLoadAsset.uri)->FinishDownloading(downLoadAsset);
            }
        }
    }
}

DBStatus RdbAssetLoader::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    DistributedData::VBucket deleteAssets = ValueProxy::Convert(std::map<std::string, Assets>{{ "", assets }});
    auto error = assetLoader_->RemoveLocalAssets("", "", {}, deleteAssets);
    return RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}
} // namespace OHOS::DistributedRdb
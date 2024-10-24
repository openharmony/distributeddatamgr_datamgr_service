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

#include <variant>

#include "error/general_error.h"
#include "log_print.h"
#include "rdb_cloud.h"
#include "store/general_value.h"
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

void RdbAssetLoader::BatchDownload(const std::string &tableName, std::vector<AssetRecord> &downloadAssets)
{
    std::vector<AssetsRecord> assetsRecords = Convert(std::move(downloadAssets));
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    PostEvent(skipAssets, assetsRecords, DistributedData::AssetEvent::DOWNLOAD, deleteAssets);
    assetLoader_->Download(tableName, assetsRecords);
    PostEvent(skipAssets, assetsRecords, DistributedData::AssetEvent::DOWNLOAD_FINISHED, deleteAssets);
    downloadAssets = Convert(std::move(assetsRecords));
}

std::vector<RdbAssetLoader::AssetsRecord> RdbAssetLoader::Convert(std::vector<AssetRecord> &&downloadAssets)
{
    std::vector<AssetsRecord> assetsRecords;
    for (auto &assetRecord : downloadAssets) {
        AssetsRecord assetsRecord;
        assetsRecord.gid = std::move(assetRecord.gid);
        DistributedDB::Type prefixTemp = assetRecord.prefix;
        assetsRecord.prefix = ValueProxy::Convert(std::move(prefixTemp));
        assetsRecord.assets = ValueProxy::Convert(std::move(assetRecord.assets));
        assetsRecords.emplace_back(std::move(assetsRecord));
    }
    return assetsRecords;
}

void RdbAssetLoader::UpdateStatus(AssetRecord &assetRecord, const VBucket &assets)
{
    for (const auto &[key, value] : assets) {
        auto downloadAssets = std::get_if<DistributedData::Assets>(&value);
        if (downloadAssets == nullptr) {
            continue;
        }
        for (const auto &asset : *downloadAssets) {
            if (assetRecord.status == DBStatus::OK) {
                assetRecord.status = ConvertStatus(static_cast<AssetStatus>(asset.status));
                return;
            }
        }
    }
}

std::vector<IAssetLoader::AssetRecord> RdbAssetLoader::Convert(std::vector<AssetsRecord> &&assetsRecords)
{
    std::vector<AssetRecord> assetRecords;
    for (auto &assetsRecord : assetsRecords) {
        AssetRecord assetRecord{
            .gid = std::move(assetsRecord.gid),
        };
        UpdateStatus(assetRecord, assetsRecord.assets);
        assetRecord.assets = ValueProxy::Convert(std::move(assetsRecord.assets));
        assetRecords.emplace_back(std::move(assetRecord));
    }
    return assetRecords;
}

DBStatus RdbAssetLoader::ConvertStatus(AssetStatus error)
{
    switch (error) {
        case AssetStatus::STATUS_NORMAL:
            return DBStatus::OK;
        case AssetStatus::STATUS_DOWNLOADING:
            return DBStatus::OK;
        default:
            ZLOGE("error:0x%{public}x", error);
            break;
    }
    return DBStatus::CLOUD_ERROR;
}

DBStatus RdbAssetLoader::RemoveLocalAssets(const std::vector<Asset> &assets)
{
    DistributedData::VBucket deleteAssets = ValueProxy::Convert(std::map<std::string, Assets>{{ "", assets }});
    auto error = assetLoader_->RemoveLocalAssets("", "", {}, deleteAssets);
    return RdbCloud::ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}

void RdbAssetLoader::PostEvent(std::set<std::string> &skipAssets, std::vector<AssetsRecord> &assetsRecords,
    DistributedData::AssetEvent eventId, std::set<std::string> &deleteAssets)
{
    for (auto &assetsRecord : assetsRecords) {
        for (auto &asset : assetsRecord.assets) {
            auto *downLoadAssets = Traits::get_if<DistributedData::Assets>(&asset.second);
            if (downLoadAssets == nullptr) {
                return;
            }
            PostEvent(eventId, *downLoadAssets, skipAssets, deleteAssets);
        }
    }
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
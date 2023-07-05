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
    return ConvertStatus(static_cast<DistributedData::GeneralError>(error));
}

DBStatus RdbAssetLoader::ConvertStatus(DistributedData::GeneralError error)
{
    switch (error) {
        case DistributedData::GeneralError::E_OK:
            return DBStatus::OK;
        case DistributedData::GeneralError::E_BUSY:
            return DBStatus::BUSY;
        case DistributedData::GeneralError::E_INVALID_ARGS:
            return DBStatus::INVALID_ARGS;
        case DistributedData::GeneralError::E_NOT_SUPPORT:
            return DBStatus::NOT_SUPPORT;
        case DistributedData::GeneralError::E_ERROR: // fallthrough
        case DistributedData::GeneralError::E_NOT_INIT:
        case DistributedData::GeneralError::E_ALREADY_CONSUMED:
        case DistributedData::GeneralError::E_ALREADY_CLOSED:
            return DBStatus::CLOUD_ERROR;
        default:
            ZLOGE("unknown error:0x%{public}x", error);
            break;
    }
    return DBStatus::CLOUD_ERROR;
}
} // namespace OHOS::DistributedRdb
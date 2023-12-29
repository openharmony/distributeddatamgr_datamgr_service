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

#define LOG_TAG "ExtensionAssetLoaderImpl"
#include "asset_loader_impl.h"
#include "extension_util.h"
#include "log_print.h"

namespace OHOS::CloudData {
AssetLoaderImpl::AssetLoaderImpl(OhCloudExtCloudAssetLoader *loader) : loader_(loader)
{
}

AssetLoaderImpl::~AssetLoaderImpl()
{
    if (loader_ != nullptr) {
        OhCloudExtCloudAssetLoaderFree(loader_);
        loader_ = nullptr;
    }
}

int32_t AssetLoaderImpl::Download(
    const std::string &tableName, const std::string &gid, const DBValue &prefix, DBVBucket &assets)
{
    std::string pref;
    auto pre = std::get_if<std::string>(&prefix);
    if (pre != nullptr) {
        pref = *pre;
    }
    OhCloudExtUpDownloadInfo downInfo {
        .tableName = reinterpret_cast<const unsigned char *>(tableName.c_str()),
        .tableNameLen = tableName.size(),
        .gid = reinterpret_cast<const unsigned char *>(gid.c_str()),
        .gidLen = gid.size(),
        .prefix = reinterpret_cast<const unsigned char *>(pref.c_str()),
        .prefixLen = pref.size()
    };
    for (auto &[key, value] : assets) {
        auto dbAssets = std::get_if<DBAssets>(&value);
        if (dbAssets == nullptr) {
            return DBErr::E_INVALID_ARGS;
        }
        auto data = ExtensionUtil::Convert(*dbAssets);
        if (data.first == nullptr) {
            return DBErr::E_ERROR;
        }
        auto status = OhCloudExtCloudAssetLoaderDownload(loader_, &downInfo, data.first);
        if (status != ERRNO_SUCCESS) {
            OhCloudExtVectorFree(data.first);
            return ExtensionUtil::ConvertStatus(status);
        }
        value = ExtensionUtil::ConvertAssets(data.first);
        OhCloudExtVectorFree(data.first);
    }
    return DBErr::E_OK;
}

int32_t AssetLoaderImpl::RemoveLocalAssets(
    const std::string &tableName, const std::string &gid, const DBValue &prefix, DBVBucket &assets)
{
    for (auto &[key, value] : assets) {
        auto dbAssets = std::get_if<DBAssets>(&value);
        if (dbAssets != nullptr) {
            for (auto &dbAsset : *dbAssets) {
                RemoveLocalAsset(dbAsset);
            }
            return DBErr::E_OK;
        }
        auto dbAsset = std::get_if<DBAsset>(&value);
        if (dbAssets != nullptr) {
            RemoveLocalAsset(*dbAsset);
            return DBErr::E_OK;
        }
        return DBErr::E_INVALID_ARGS;
    }
    return DBErr::E_OK;
}

int32_t AssetLoaderImpl::RemoveLocalAsset(const DBAsset &dbAsset)
{
    auto data = ExtensionUtil::Convert(dbAsset);
    if (data.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudAssetLoaderRemoveLocalAssets(data.first);
    OhCloudExtCloudAssetFree(data.first);
    if (status != ERRNO_SUCCESS) {
        return DBErr::E_ERROR;
    }
    return DBErr::E_OK;
}
} // namespace OHOS::CloudData
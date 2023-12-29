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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_ASSET_LOADER_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_ASSET_LOADER_IMPL_H

#include "cloud/asset_loader.h"
#include "cloud/schema_meta.h"
#include "cloud_extension.h"

namespace OHOS::CloudData {
using DBValue = DistributedData::Value;
using DBVBucket = DistributedData::VBucket;
using DBMeta = DistributedData::Database;
using DBAssets = DistributedData::Assets;
using DBAsset = DistributedData::Asset;
using DBErr = DistributedData::GeneralError;
class AssetLoaderImpl : public DistributedData::AssetLoader {
public:
    explicit AssetLoaderImpl(OhCloudExtCloudAssetLoader *loader);
    ~AssetLoaderImpl();
    int32_t Download(const std::string &tableName, const std::string &gid, const DBValue &prefix,
        DBVBucket &assets) override;
    int32_t RemoveLocalAssets(const std::string &tableName, const std::string &gid,
        const DBValue &prefix, DBVBucket &assets) override;

private:
    OhCloudExtCloudAssetLoader *loader_ = nullptr;
    int32_t RemoveLocalAsset(const DBAsset &dbAsset);
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_ASSET_LOADER_IMPL_H

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
#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_DATA_TRASLATE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_DATA_TRASLATE_H

#include "cloud/icloud_data_translate.h"
#include "serializable/serializable.h"
#include "value_object.h"
namespace OHOS::DistributedRdb {
class RdbCloudDataTranslate : public DistributedDB::ICloudDataTranslate {
public:
    using Asset = DistributedDB::Asset;
    using Assets = DistributedDB::Assets;
    using DataAsset = NativeRdb::ValueObject::Asset;
    using DataAssets = NativeRdb::ValueObject::Assets;
    RdbCloudDataTranslate() = default;
    ~RdbCloudDataTranslate() = default;
    std::vector<uint8_t> AssetToBlob(const Asset &asset) override;
    std::vector<uint8_t> AssetsToBlob(const Assets &assets) override;
    Asset BlobToAsset(const std::vector<uint8_t> &blob) override;
    Assets BlobToAssets(const std::vector<uint8_t> &blob) override;

private:
    using Serializable = DistributedData::Serializable;

    static constexpr const uint32_t ASSET_MAGIC = 0x41534554;
    static constexpr const uint32_t ASSETS_MAGIC = 0x41534553;
    struct InnerAsset : public Serializable {
        DataAsset &asset_;
        explicit InnerAsset(DataAsset &asset) : asset_(asset) {}

        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };
    size_t ParserRawData(const uint8_t *data, size_t length, DataAsset &asset);
    size_t ParserRawData(const uint8_t *data, size_t length, DataAssets &assets);
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_DATA_TRASLATE_H

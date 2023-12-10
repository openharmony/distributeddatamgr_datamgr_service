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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_SNAPSHOT_H
#define DISTRIBUTEDDATAMGR_OBJECT_SNAPSHOT_H

#include "object_asset_machine.h"
#include "snapshot/snapshot.h"
#include "store/general_value.h"
namespace OHOS {
namespace DistributedObject {
using namespace DistributedData;
class ObjectSnapshot : public Snapshot {
public:
    ObjectSnapshot();
    ~ObjectSnapshot() override;

    int32_t Upload(Asset& asset) override;
    int32_t Download(Asset& asset) override;
    TransferStatus GetAssetStatus(Asset& asset) override;
    int32_t Uploaded(Asset& asset) override;
    int32_t Downloaded(Asset& asset) override;
    int32_t Transferred(Asset& asset) override;
    int32_t OnDataChanged(Asset& asset, const std::string& deviceId) override;
    int32_t BindAsset(const Asset& asset, const DistributedData::AssetBindInfo& bindInfo,
        const StoreInfo& storeInfo) override;
    bool IsBoundAsset(const Asset& asset) override;

private:
    std::map<std::string, ChangedAssetInfo> changedAssets_;
    std::shared_ptr<ObjectAssetMachine> assetMachine_;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_SNAPSHOT_H

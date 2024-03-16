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


#ifndef DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H
#define DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H

#include "snapshot/machine_status.h"
#include "store/store_info.h"

namespace OHOS {
namespace DistributedObject {
using namespace OHOS::DistributedData;
struct ChangedAssetInfo {
    ChangedAssetInfo() = default;
    std::string deviceId;
    DistributedData::Asset asset;
    DistributedData::TransferStatus status = DistributedData::STATUS_STABLE;
    AssetBindInfo bindInfo;
    StoreInfo storeInfo;

    ChangedAssetInfo(const Asset& bindAsset, const AssetBindInfo& assetBindInfo, const StoreInfo& store)
    {
        asset = bindAsset;
        bindInfo = assetBindInfo;
        storeInfo = store;
    }
};

typedef int32_t (*Action)
    (int32_t eventId, ChangedAssetInfo& changedAsset, Asset& asset, const std::pair<std::string, Asset>& newAsset);

struct DFAAction {
    int32_t next;
    Action before;
    Action after;
};

class ObjectAssetMachine {
public:
    ObjectAssetMachine();

    static int32_t DFAPostEvent(AssetEvent eventId, ChangedAssetInfo& changedAsset, Asset& asset,
        const std::pair<std::string, Asset>& newAsset = std::pair<std::string, Asset>());

private:
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_ASSET_MACHINE_H

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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_SNAPSHOT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_SNAPSHOT_H
#include "machine_status.h"
#include "store/general_value.h"
#include "store/store_info.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedData {

class API_EXPORT Snapshot {
public:
    virtual ~Snapshot() = default;

    virtual int32_t Transferred(Asset& asset) = 0;

    virtual int32_t Upload(Asset& asset) = 0;

    virtual int32_t Download(Asset& asset) = 0;

    virtual int32_t Uploaded(Asset& asset) = 0;

    virtual int32_t Downloaded(Asset& asset) = 0;

    virtual TransferStatus GetAssetStatus(Asset& asset) = 0;

    virtual int32_t BindAsset(const Asset& asset, const AssetBindInfo& bindInfo, const StoreInfo& storeInfo) = 0;

    virtual int32_t OnDataChanged(Asset& asset, const std::string &deviceId) = 0;

    virtual bool IsBoundAsset(const Asset& asset) = 0;

};

struct BindAssets {
    std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_SNAPSHOT_H

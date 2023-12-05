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
#include "visibility.h"
namespace OHOS {
namespace DistributedData {

class API_EXPORT Snapshot {
public:
    virtual ~Snapshot() = default;

    virtual int32_t FinishTransferring(Asset& asset);

    virtual int32_t Upload(Asset& asset);

    virtual int32_t Download(Asset& asset);

    virtual int32_t FinishUploading(Asset& asset);

    virtual int32_t FinishDownloading(Asset& asset);

    virtual TransferStatus GetAssetStatus(Asset& asset);

    virtual int32_t BindAsset(const Asset& asset, const RdbBindInfo& bindInfo, const StoreInfo& storeInfo);

    virtual int32_t OnDataChanged(Asset& asset,const std::string &deviceId);

    virtual bool IsBindAsset(const Asset& asset);
};

struct Snapshots {
    std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> snapshots;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_SNAPSHOT_H

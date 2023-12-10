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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_MACHINE_STATUS_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_MACHINE_STATUS_H

#include "store/general_value.h"

namespace OHOS {
namespace DistributedData {

enum TransferStatus : int32_t {
    STATUS_STABLE = 0,
    STATUS_TRANSFERRING,
    STATUS_DOWNLOADING,
    STATUS_UPLOADING,
    STATUS_WAIT_TRANSFER,
    STATUS_WAIT_UPLOAD,
    STATUS_WAIT_DOWNLOAD,
    STATUS_BUTT,
    STATUS_NO_CHANGE = 0XFFFFFFF,
};

enum AssetEvent {
    REMOTE_CHANGED = 0,
    TRANSFER_FINISHED,
    UPLOAD,
    UPLOAD_FINISHED,
    DOWNLOAD,
    DOWNLOAD_FINISHED,
    EVENT_BUTT,
};

struct AssetBindInfo {
    std::string storeName;
    std::string tableName;
    DistributedData::VBucket primaryKey;
    std::string field;
    std::string assetName;
};

struct StoreInfo {
    uint32_t tokenId = 0;
    int32_t instanceId = 0;
    int32_t user;
    std::string bundleName;
    std::string storeName;
};
} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SNAPSHOT_MACHINE_STATUS_H

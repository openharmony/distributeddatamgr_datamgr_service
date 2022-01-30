/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
struct StoreMetaData final : public Serializable {
    bool isAutoSync = false;
    bool isBackup = false;
    bool isDirty = false;
    bool isEncrypt = false;
    int32_t kvStoreType = 0;
    int32_t securityLevel = 0;
    int32_t uid = -1;
    std::string appId = "";
    std::string appType = "";
    std::string bundleName = "";
    std::string dataDir = "";
    std::string deviceAccountId = ""; // todo change to userId
    std::string deviceId = "";
    std::string schema = "";
    std::string storeId = "";
    std::string userId = "";
    uint32_t version = 0;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};
}
}
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_STORE_META_DATA_H

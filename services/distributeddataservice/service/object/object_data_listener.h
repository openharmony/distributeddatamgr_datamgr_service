/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_DATA_LISTENER_H
#define DISTRIBUTEDDATAMGR_OBJECT_DATA_LISTENER_H

#include "kv_store_observer.h"
#include "asset/asset_recv_callback_stub.h"
namespace OHOS {
namespace DistributedObject {
using AssetObj = Storage::DistributedFile::AssetObj;
class ObjectDataListener : public DistributedDB::KvStoreObserver {
public:
    ObjectDataListener();
    ~ObjectDataListener();
    // Database change callback
    void OnChange(const DistributedDB::KvStoreChangedData &data) override;
};

class ObjectAssetsRecvListener : public Storage::DistributedFile::AssetRecvCallbackStub {
public:
    ObjectAssetsRecvListener() =default;
    ~ObjectAssetsRecvListener() =default;
    int32_t OnStart(const std::string &srcNetworkId,
                  const std::string &dstNetworkId, const std::string &sessionId,
                  const std::string &dstBundleName) override;
    int32_t OnFinished(const std::string &srcNetworkId,
                     const sptr<AssetObj> &assetObj, int32_t result) override;
};
}  // namespace DistributedObject
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_DATA_LISTENER_H

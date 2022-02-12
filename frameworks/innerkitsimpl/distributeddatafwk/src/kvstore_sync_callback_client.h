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

#ifndef KVSTORE_SYNC_CALLBACK_CLIENT_H
#define KVSTORE_SYNC_CALLBACK_CLIENT_H

#include <mutex>
#include "ikvstore_sync_callback.h"
#include "kvstore_sync_callback.h"

namespace OHOS {
namespace DistributedKv {

class KvStoreSyncCallbackClient : public KvStoreSyncCallbackStub {
public:
    virtual ~KvStoreSyncCallbackClient();

    void SyncCompleted(const std::map<std::string, Status> &results, const std::string &label) override;

    void AddKvStoreSyncCallback(const std::shared_ptr<KvStoreSyncCallback> kvStoreSyncCallback,
                                const std::string &label);

    void DeleteCommonKvStoreSyncCallback();

    std::string GetCommonSyncCallbackLabel();

    static sptr<KvStoreSyncCallbackClient> GetInstance()
    {
        static sptr<KvStoreSyncCallbackClient> pInstance_;
        return pInstance_;
    }
private:
    KvStoreSyncCallbackClient();
    static std::map<std::string, std::shared_ptr<KvStoreSyncCallback>> kvStoreSyncCallbackInfo_;
    static std::mutex syncCallbackMutex_;
    const std::string CommonSyncCallbackLabel = "CommonSyncCallbackLabel";
//    static sptr<KvStoreSyncCallbackClient> pInstance_;
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // KVSTORE_SYNC_CALLBACK_CLIENT_H

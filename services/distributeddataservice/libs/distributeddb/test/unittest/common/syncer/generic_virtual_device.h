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
#ifndef GENERIC_VIRTUAL_DEVICE_H
#define GENERIC_VIRTUAL_DEVICE_H

#include <string>

#include "ikvdb_sync_interface.h"
#include "meta_data.h"
#include "subscribe_manager.h"
#include "sync_task_context.h"
#include "types.h"
#include "virtual_communicator_aggregator.h"

namespace DistributedDB {
class GenericVirtualDevice {
public:
    explicit GenericVirtualDevice(std::string deviceId);
    virtual ~GenericVirtualDevice();

    int Initialize(VirtualCommunicatorAggregator *comAggregator, ISyncInterface *syncInterface);
    void SetDeviceId(const std::string &deviceId);
    std::string GetDeviceId() const;
    int MessageCallback(const std::string &deviceId, Message *inMsg);
    void OnRemoteDataChanged(const std::function<void(const std::string &)> &callback);
    void Online();
    void Offline();
    int StartResponseTask();
    TimeOffset GetLocalTimeOffset() const;
    virtual int Sync(SyncMode mode, bool wait);
    virtual int Sync(SyncMode mode, const Query &query, bool wait);
    virtual int Sync(SyncMode mode, const Query &query, const SyncOperation::UserCallback &callBack, bool wait);

protected:
    ICommunicator *communicateHandle_;
    VirtualCommunicatorAggregator *communicatorAggregator_;
    ISyncInterface *storage_;
    std::shared_ptr<Metadata> metadata_;
    std::string deviceId_;
    std::string remoteDeviceId_;
    SyncTaskContext *context_;
    std::function<void(const std::string &)> onRemoteDataChanged_;

    std::shared_ptr<SubscribeManager> subManager_;
};
}
#endif // GENERIC_VIRTUAL_DEVICE_H

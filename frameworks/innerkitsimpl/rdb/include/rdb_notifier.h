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

#ifndef DISTRIBUTED_RDB_NOTIFIER_H
#define DISTRIBUTED_RDB_NOTIFIER_H

#include <iremote_broker.h>
#include <iremote_proxy.h>
#include <iremote_stub.h>

#include "rdb_types.h"

namespace OHOS::DistributedRdb {
class IRdbNotifier: public IRemoteBroker {
public:
    enum {
        RDB_NOTIFIER_CMD_SYNC_COMPLETE,
        RDB_NOTIFIER_CMD_DATA_CHANGE,
        RDB_NOTIFIER_CMD_MAX
    };
    
    virtual int32_t OnComplete(uint32_t seqNum, const SyncResult& result) = 0;
    
    virtual int32_t OnChange(const std::string& storeName, const std::vector<std::string>& devices) = 0;
    
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbNotifier");
};

class RdbNotifierProxy : public IRemoteProxy<IRdbNotifier> {
public:
    explicit RdbNotifierProxy(const sptr<IRemoteObject>& object);
    virtual ~RdbNotifierProxy() noexcept;
    
    int32_t OnComplete(uint32_t seqNum, const SyncResult& result) override;
    
    int32_t OnChange(const std::string& storeName, const std::vector<std::string>& devices) override;

private:
    static inline BrokerDelegator<RdbNotifierProxy> delegator_;
};

using RdbSyncCompleteNotifier = std::function<void(uint32_t, const SyncResult&)>;
using RdbDataChangeNotifier = std::function<void(const std::string&, const std::vector<std::string>&)>;

class RdbNotifierStub : public IRemoteStub<IRdbNotifier> {
public:
    RdbNotifierStub(const RdbSyncCompleteNotifier&, const RdbDataChangeNotifier&);
    virtual ~RdbNotifierStub() noexcept;
    
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
    
    int32_t OnCompleteInner(MessageParcel& data, MessageParcel& reply);
    int32_t OnComplete(uint32_t seqNum, const SyncResult& result) override;
    
    int32_t OnChangeInner(MessageParcel&data, MessageParcel& reply);
    int32_t OnChange(const std::string& storeName, const std::vector<std::string>& devices) override;
    
private:
    bool CheckInterfaceToken(MessageParcel& data);
    
    using RequestHandle = int32_t (RdbNotifierStub::*)(MessageParcel&, MessageParcel&);
    static constexpr RequestHandle HANDLES[RDB_NOTIFIER_CMD_MAX] = {
        [RDB_NOTIFIER_CMD_SYNC_COMPLETE] = &RdbNotifierStub::OnCompleteInner,
        [RDB_NOTIFIER_CMD_DATA_CHANGE] = &RdbNotifierStub::OnChangeInner,
    };
    
    RdbSyncCompleteNotifier completeNotifier_;
    RdbDataChangeNotifier changeNotifier_;
};
}
#endif

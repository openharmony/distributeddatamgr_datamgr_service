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

#define LOG_TAG "RdbServiceProxy"

#include "rdb_service_proxy.h"
#include "itypes_util.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {
RdbServiceProxy::RdbServiceProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IRdbService>(object)
{
    ZLOGI("construct");
}

void RdbServiceProxy::OnSyncComplete(uint32_t seqNum, const SyncResult &result)
{
    syncCallbacks_.ComputeIfPresent(seqNum, [&result] (const auto& key, const SyncCallback& callback) {
        callback(result);
        return true;
    });
    syncCallbacks_.Erase(seqNum);
}

void RdbServiceProxy::OnDataChange(const std::string& storeName, const std::vector<std::string> &devices)
{
    ZLOGI("%{public}s", storeName.c_str());
    observers_.ComputeIfPresent(
        storeName, [&devices] (const auto& key, const ObserverMapValue& value) {
            for (const auto& observer : value.first) {
                observer->OnChange(devices);
            }
            return true;
        });
}

std::string RdbServiceProxy::ObtainDistributedTableName(const std::string &device, const std::string &table)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return "";
    }
    if (!DistributedKv::ITypesUtil::Marshalling(data, device, table)) {
        ZLOGE("write table failed");
        return "";
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_OBTAIN_TABLE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return "";
    }
    return reply.ReadString();
}

int32_t RdbServiceProxy::InitNotifier(const RdbSyncerParam& param)
{
    notifier_ = new (std::nothrow) RdbNotifierStub(
        [this] (uint32_t seqNum, const SyncResult& result) {
            OnSyncComplete(seqNum, result);
        },
        [this] (const std::string& storeName, const std::vector<std::string>& devices) {
            OnDataChange(storeName, devices);
        }
    );
    if (notifier_ == nullptr) {
        ZLOGE("create notifier failed");
        return RDB_ERROR;
    }

    if (InitNotifier(param, notifier_->AsObject()) != RDB_OK) {
        notifier_ = nullptr;
        return RDB_ERROR;
    }

    ZLOGI("success");
    return RDB_OK;
}

int32_t RdbServiceProxy::InitNotifier(const RdbSyncerParam &param, const sptr<IRemoteObject> notifier)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(data, param, notifier)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_INIT_NOTIFIER, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    int32_t res = RDB_ERROR;
    return reply.ReadInt32(res) ? res : RDB_ERROR;
}

uint32_t RdbServiceProxy::GetSeqNum()
{
    return seqNum_++;
}

int32_t RdbServiceProxy::DoSync(const RdbSyncerParam& param, const SyncOption &option,
                                const RdbPredicates &predicates, SyncResult& result)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(data, param, option, predicates)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption opt;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_SYNC, data, reply, opt) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    if (!DistributedKv::ITypesUtil::Unmarshalling(reply, result)) {
        ZLOGE("read result failed");
        return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbServiceProxy::DoSync(const RdbSyncerParam& param, const SyncOption &option,
                                const RdbPredicates &predicates, const SyncCallback& callback)
{
    SyncResult result;
    if (DoSync(param, option, predicates, result) != RDB_OK) {
        ZLOGI("failed");
        return RDB_ERROR;
    }
    ZLOGI("success");

    if (callback != nullptr) {
        callback(result);
    }
    return RDB_OK;
}

int32_t RdbServiceProxy::DoAsync(const RdbSyncerParam& param, uint32_t seqNum, const SyncOption &option,
                                 const RdbPredicates &predicates)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(data, param, seqNum, option, predicates)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption opt;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_ASYNC, data, reply, opt) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    int32_t res = RDB_ERROR;
    return reply.ReadInt32(res) ? res : RDB_ERROR;
}

int32_t RdbServiceProxy::DoAsync(const RdbSyncerParam& param, const SyncOption &option,
                                 const RdbPredicates &predicates, const SyncCallback& callback)
{
    uint32_t num = GetSeqNum();
    if (!syncCallbacks_.Insert(num, callback)) {
        ZLOGI("insert callback failed");
        return RDB_ERROR;
    }
    ZLOGI("num=%{public}u", num);

    if (DoAsync(param, num, option, predicates) != RDB_OK) {
        ZLOGE("failed");
        syncCallbacks_.Erase(num);
        return RDB_ERROR;
    }

    ZLOGI("success");
    return RDB_OK;
}

int32_t RdbServiceProxy::SetDistributedTables(const RdbSyncerParam& param, const std::vector<std::string> &tables)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(data, param, tables)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_SET_DIST_TABLE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    int32_t res = RDB_ERROR;
    return reply.ReadInt32(res) ? res : RDB_ERROR;
}

int32_t RdbServiceProxy::Sync(const RdbSyncerParam& param, const SyncOption &option,
                              const RdbPredicates &predicates, const SyncCallback &callback)
{
    if (option.isBlock) {
        return DoSync(param, option, predicates, callback);
    }
    return DoAsync(param, option, predicates, callback);
}

int32_t RdbServiceProxy::Subscribe(const RdbSyncerParam &param, const SubscribeOption &option,
                                   RdbStoreObserver *observer)
{
    if (option.mode != SubscribeMode::REMOTE) {
        ZLOGE("subscribe mode invalid");
        return RDB_ERROR;
    }
    if (DoSubscribe(param) != RDB_OK) {
        ZLOGI("communicate to server failed");
        return RDB_ERROR;
    }
    observers_.Compute(
        param.storeName_, [observer] (const auto& key, ObserverMapValue& value) {
            for (const auto& element : value.first) {
                if (element == observer) {
                    ZLOGE("duplicate observer");
                    return true;
                }
            }
            value.first.push_back(observer);
            return true;
        });
    return RDB_OK;
}

int32_t RdbServiceProxy::DoSubscribe(const RdbSyncerParam &param)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(param, data)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_SUBSCRIBE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    int32_t res = RDB_ERROR;
    return reply.ReadInt32(res) ? res : RDB_ERROR;
}

int32_t RdbServiceProxy::UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
                                     RdbStoreObserver *observer)
{
    DoUnSubscribe(param);
    observers_.ComputeIfPresent(
        param.storeName_, [observer](const auto& key, ObserverMapValue& value) {
            ZLOGI("before remove size=%{public}d", static_cast<int>(value.first.size()));
            value.first.remove(observer);
            ZLOGI("after  remove size=%{public}d", static_cast<int>(value.first.size()));
            return !(value.first.empty());
    });
    return RDB_OK;
}

int32_t RdbServiceProxy::DoUnSubscribe(const RdbSyncerParam &param)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(IRdbService::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!DistributedKv::ITypesUtil::Marshalling(param, data)) {
        ZLOGE("write param failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(RDB_SERVICE_CMD_UNSUBSCRIBE, data, reply, option) != 0) {
        ZLOGE("send request failed");
        return RDB_ERROR;
    }

    int32_t res = RDB_ERROR;
    return reply.ReadInt32(res) ? res : RDB_ERROR;
}

RdbServiceProxy::ObserverMap RdbServiceProxy::ExportObservers()
{
    return observers_;
}

void RdbServiceProxy::ImportObservers(ObserverMap &observers)
{
    ZLOGI("enter");
    SubscribeOption option {SubscribeMode::REMOTE};
    observers.ForEach([this, &option](const std::string& key, const ObserverMapValue& value) {
        for (auto& observer : value.first) {
            Subscribe(value.second, option, observer);
        }
        return false;
    });
}
} // namespace OHOS::DistributedRdb

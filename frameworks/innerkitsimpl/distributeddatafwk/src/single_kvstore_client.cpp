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

#define LOG_TAG "SingleKvStoreClient"

#include "single_kvstore_client.h"
#include "constant.h"
#include "dds_trace.h"
#include "kvstore_observer_client.h"
#include "kvstore_resultset_client.h"
#include "kvstore_sync_callback_client.h"
#include "log_print.h"
#include "kvstore_utils.h"

namespace OHOS::DistributedKv {
SingleKvStoreClient::SingleKvStoreClient(sptr<ISingleKvStore> kvStoreProxy, const std::string &storeId)
    : kvStoreProxy_(kvStoreProxy), storeId_(storeId), syncCallbackClient_(new KvStoreSyncCallbackClient()),
      syncObserver_(std::make_shared<SyncObserver>())
{}

SingleKvStoreClient::~SingleKvStoreClient()
{
    kvStoreProxy_->UnRegisterSyncCallback();
    syncObserver_->Clean();
}

StoreId SingleKvStoreClient::GetStoreId() const
{
    StoreId storeId;
    storeId.storeId = storeId_;
    return storeId;
}

Status SingleKvStoreClient::GetEntries(const Key &prefix, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }

    return kvStoreProxy_->GetEntries(prefix, entries);
}

Status SingleKvStoreClient::GetEntries(const DataQuery &query, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    ZLOGD("Cpp client GetEntries");
    return kvStoreProxy_->GetEntriesWithQuery(query.ToString(), entries);
}

Status SingleKvStoreClient::GetResultSet(const Key &prefix, std::shared_ptr<KvStoreResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    resultSet = nullptr;
    Status statusTmp = Status::SERVER_UNAVAILABLE;
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return statusTmp;
    }
    sptr<IKvStoreResultSet> resultSetTmp;
    auto callFun = [&](Status status, sptr<IKvStoreResultSet> proxy) {
        statusTmp = status;
        resultSetTmp = proxy;
    };
    kvStoreProxy_->GetResultSet(prefix, callFun);
    if (statusTmp != Status::SUCCESS) {
        ZLOGE("return error: %d.", static_cast<int>(statusTmp));
        return statusTmp;
    }

    if (resultSetTmp == nullptr) {
        ZLOGE("resultSetTmp is nullptr.");
        return statusTmp;
    }

    resultSet = std::shared_ptr<KvStoreResultSetClient>(
        new KvStoreResultSetClient(resultSetTmp), [proxy = kvStoreProxy_] (auto result) {
        ZLOGE("kvstore proxy ResultSet closed.");
        proxy->CloseResultSet(result->GetKvStoreResultSetProxy());
    });
    return statusTmp;
}

Status SingleKvStoreClient::GetResultSet(const DataQuery &query,
                                         std::shared_ptr<KvStoreResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    resultSet = nullptr;
    Status statusTmp = Status::SERVER_UNAVAILABLE;
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return statusTmp;
    }

    ZLOGD("Cpp client GetResultSet");
    sptr<IKvStoreResultSet> resultSetTmp;
    auto callFun = [&](Status status, sptr<IKvStoreResultSet> proxy) {
        statusTmp = status;
        resultSetTmp = proxy;
    };
    kvStoreProxy_->GetResultSetWithQuery(query.ToString(), callFun);
    if (statusTmp != Status::SUCCESS) {
        ZLOGE("return error: %d.", static_cast<int>(statusTmp));
        return statusTmp;
    }

    if (resultSetTmp == nullptr) {
        ZLOGE("resultSetTmp is nullptr.");
        return statusTmp;
    }
    resultSet = std::shared_ptr<KvStoreResultSetClient>(
        new KvStoreResultSetClient(resultSetTmp), [proxy = kvStoreProxy_] (auto result) {
        ZLOGE("kvstore proxy ResultSetwithquery closed.");
        proxy->CloseResultSet(result->GetKvStoreResultSetProxy());
    });
    return statusTmp;
}

Status SingleKvStoreClient::CloseResultSet(std::shared_ptr<KvStoreResultSet> &resultSet)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    auto resultSetTmp = std::move(resultSet);
    if (resultSetTmp == nullptr) {
        ZLOGE("resultSet is nullptr.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    auto resultSetClient = reinterpret_cast<KvStoreResultSetClient *>(resultSetTmp.get());
    return kvStoreProxy_->CloseResultSet(resultSetClient->GetKvStoreResultSetProxy());
}

Status SingleKvStoreClient::GetCount(const DataQuery &query, int &count) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    ZLOGD("Cpp client GetCount");
    return kvStoreProxy_->GetCountWithQuery(query.ToString(), count);
}

Status SingleKvStoreClient::Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON | TraceSwitch::TRACE_CHAIN_ON);
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    if (devices.empty()) {
        ZLOGW("deviceIds is empty.");
        return Status::INVALID_ARGUMENT;
    }
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    syncCallbackClient_->AddSyncCallback(syncObserver_, sequenceId);
    RegisterCallback();
    return kvStoreProxy_->Sync(devices, mode, delay, sequenceId);
}

Status SingleKvStoreClient::RemoveDeviceData(const std::string &device)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    if (device.empty()) {
        ZLOGW("device is empty.");
        return Status::INVALID_ARGUMENT;
    }
    return kvStoreProxy_->RemoveDeviceData(device);
}

Status SingleKvStoreClient::Delete(const Key &key)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    ZLOGI("begin.");
    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    if (keyData.size() == 0 || keyData.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGE("invalid key.");
        return Status::INVALID_ARGUMENT;
    }

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    return kvStoreProxy_->Delete(key);
}

Status SingleKvStoreClient::Put(const Key &key, const Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    ZLOGI("key: %zu value: %zu.", key.Size(), value.Size());
    std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    if (keyData.size() == 0 || keyData.size() > Constant::MAX_KEY_LENGTH ||
        value.Size() > Constant::MAX_VALUE_LENGTH) {
        ZLOGE("invalid key or value.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    return kvStoreProxy_->Put(key, value);
}

Status SingleKvStoreClient::Get(const Key &key, Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ == nullptr) {
        ZLOGE("kvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    return kvStoreProxy_->Get(key, value);
}

Status SingleKvStoreClient::SubscribeKvStore(SubscribeType subscribeType, std::shared_ptr<KvStoreObserver> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    if (observer == nullptr) {
        ZLOGW("return INVALID_ARGUMENT.");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lck(observerMapMutex_);
    // change this to map.contains() after c++20
    if (registeredObservers_.count(observer.get()) == 1) {
        ZLOGW("return STORE_ALREADY_SUBSCRIBE.");
        return Status::STORE_ALREADY_SUBSCRIBE;
    }
    // remove storeId after remove SubscribeKvStore function in manager. currently reserve for convenience.
    sptr<KvStoreObserverClient> ipcObserver = new (std::nothrow) KvStoreObserverClient(observer);
    if (ipcObserver == nullptr) {
        ZLOGW("new KvStoreObserverClient failed");
        return Status::ERROR;
    }
    Status status = kvStoreProxy_->SubscribeKvStore(subscribeType, ipcObserver);
    if (status == Status::SUCCESS) {
        const auto temp = registeredObservers_.insert({observer.get(), ipcObserver});
        if (!temp.second) {
            ZLOGW("local insert error");
            return Status::ERROR;
        }
    }
    return status;
}

Status SingleKvStoreClient::UnSubscribeKvStore(SubscribeType subscribeType, std::shared_ptr<KvStoreObserver> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    if (observer == nullptr) {
        ZLOGW("return INVALID_ARGUMENT.");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lck(observerMapMutex_);
    auto it = registeredObservers_.find(observer.get());
    if (it == registeredObservers_.end()) {
        ZLOGW(" STORE NOT SUBSCRIBE.");
        return Status::STORE_NOT_SUBSCRIBE;
    }
    Status status = kvStoreProxy_->UnSubscribeKvStore(subscribeType, it->second);
    if (status == Status::SUCCESS) {
        registeredObservers_.erase(it);
    } else {
        ZLOGW("single unSubscribe failed code=%d.", static_cast<int>(status));
    }
    return status;
}

Status SingleKvStoreClient::RegisterSyncCallback(std::shared_ptr<KvStoreSyncCallback> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    ZLOGI("begin.");
    if (callback == nullptr) {
        ZLOGW("return INVALID_ARGUMENT.");
        return Status::INVALID_ARGUMENT;
    }
    syncObserver_->Add(callback);
    RegisterCallback();
    return Status::SUCCESS;
}

Status SingleKvStoreClient::RegisterCallback()
{
    if (isRegisterSyncCallback_) {
        return Status::SUCCESS;
    }
    std::lock_guard lg(registerCallbackMutex_);
    if (isRegisterSyncCallback_) {
        return Status::SUCCESS;
    }
    auto status = kvStoreProxy_->RegisterSyncCallback(syncCallbackClient_);
    if (status != Status::SUCCESS) {
        ZLOGE("RegisterSyncCallback is not success.");
        return status;
    }
    isRegisterSyncCallback_ = true;
    return Status::SUCCESS;
}

Status SingleKvStoreClient::UnRegisterSyncCallback()
{
    ZLOGI("begin.");
    syncObserver_->Clean();
    return Status::SUCCESS;
}

Status SingleKvStoreClient::PutBatch(const std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    ZLOGI("entry size: %zu", entries.size());
    if (entries.size() > Constant::MAX_BATCH_SIZE) {
        ZLOGE("batch size must less than 128.");
        return Status::INVALID_ARGUMENT;
    }
    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->PutBatch(entries);
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::DeleteBatch(const std::vector<Key> &keys)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    if (keys.size() > Constant::MAX_BATCH_SIZE) {
        ZLOGE("batch size must less than 128.");
        return Status::INVALID_ARGUMENT;
    }

    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->DeleteBatch(keys);
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::StartTransaction()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->StartTransaction();
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::Commit()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->Commit();
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::Rollback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);

    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->Rollback();
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::SetSyncParam(const KvSyncParam &syncParam)
{
    KvParam input(TransferTypeToByteArray<uint32_t>(syncParam.allowedDelayMs));
    KvParam output;
    return Control(KvControlCmd::SET_SYNC_PARAM, input, output);
}

Status SingleKvStoreClient::GetSyncParam(KvSyncParam &syncParam)
{
    KvParam inputEmpty;
    KvParam output;
    Status ret = Control(KvControlCmd::GET_SYNC_PARAM, inputEmpty, output);
    if (ret != Status::SUCCESS) {
        return ret;
    }
    if (output.Size() == sizeof(uint32_t)) {
        syncParam.allowedDelayMs = TransferByteArrayToType<uint32_t>(output.Data());
        return Status::SUCCESS;
    }
    return Status::ERROR;
}

Status SingleKvStoreClient::Control(KvControlCmd cmd, const KvParam &inputParam, KvParam &output)
{
    ZLOGI("begin.");
    if (kvStoreProxy_ != nullptr) {
        sptr<KvParam> kvParam;
        Status status = kvStoreProxy_->Control(cmd, inputParam, kvParam);
        if ((status == Status::SUCCESS) && (kvParam != nullptr)) {
            output = *kvParam;
        }
        return status;
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}
Status SingleKvStoreClient::SetCapabilityEnabled(bool enabled) const
{
    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->SetCapabilityEnabled(enabled);
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::SetCapabilityRange(const std::vector<std::string> &localLabels,
                                               const std::vector<std::string> &remoteLabels) const
{
    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->SetCapabilityRange(localLabels, remoteLabels);
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::GetSecurityLevel(SecurityLevel &secLevel) const
{
    if (kvStoreProxy_ != nullptr) {
        return kvStoreProxy_->GetSecurityLevel(secLevel);
    }
    ZLOGE("singleKvstore proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status SingleKvStoreClient::Sync(const std::vector<std::string> &devices, SyncMode mode,
                                 const DataQuery &query, std::shared_ptr<KvStoreSyncCallback> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("singleKvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    if (devices.empty()) {
        ZLOGW("deviceIds is empty.");
        return Status::INVALID_ARGUMENT;
    }
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    if (callback != nullptr) {
        syncCallbackClient_->AddSyncCallback(callback, sequenceId);
    } else {
        syncCallbackClient_->AddSyncCallback(syncObserver_, sequenceId);
    }
    RegisterCallback();
    return kvStoreProxy_->Sync(devices, mode, query.ToString(), sequenceId);
}

Status SingleKvStoreClient::SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("singleKvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    if (devices.empty()) {
        ZLOGW("deviceIds is empty.");
        return Status::INVALID_ARGUMENT;
    }
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    syncCallbackClient_->AddSyncCallback(syncObserver_, sequenceId);
    RegisterCallback();
    return kvStoreProxy_->Subscribe(devices, query.ToString(), sequenceId);
}

Status SingleKvStoreClient::UnsubscribeWithQuery(const std::vector<std::string> &deviceIds, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    if (kvStoreProxy_ == nullptr) {
        ZLOGE("singleKvstore proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }
    if (deviceIds.empty()) {
        ZLOGW("deviceIds is empty.");
        return Status::INVALID_ARGUMENT;
    }
    uint64_t sequenceId = KvStoreUtils::GenerateSequenceId();
    syncCallbackClient_->AddSyncCallback(syncObserver_, sequenceId);
    return kvStoreProxy_->UnSubscribe(deviceIds, query.ToString(), sequenceId);
}
} // namespace OHOS::DistributedKv

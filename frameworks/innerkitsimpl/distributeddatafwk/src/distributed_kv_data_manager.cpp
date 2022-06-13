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

#define LOG_TAG "DistributedKvDataManager"

#include "distributed_kv_data_manager.h"
#include "constant.h"
#include "ikvstore_data_service.h"
#include "kvstore_service_death_notifier.h"
#include "log_print.h"
#include "refbase.h"
#include "single_kvstore_client.h"
#include "dds_trace.h"
#include "communication_provider.h"
#include "device_status_change_listener_client.h"

namespace OHOS {
namespace DistributedKv {
DistributedKvDataManager::DistributedKvDataManager()
{}

DistributedKvDataManager::~DistributedKvDataManager()
{}

Status DistributedKvDataManager::GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                                  std::shared_ptr<SingleKvStore> &singleKvStore)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    singleKvStore = nullptr;
    std::string storeIdTmp = Constant::TrimCopy<std::string>(storeId.storeId);
    if (storeIdTmp.size() == 0 || storeIdTmp.size() > Constant::MAX_STORE_ID_LENGTH) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }

    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    Status status = Status::SERVER_UNAVAILABLE;
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return status;
    }

    ZLOGD("call proxy.");
    sptr<ISingleKvStore> proxyTmp;
    status = kvDataServiceProxy->GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> proxy) { proxyTmp = std::move(proxy); });
    if (status == Status::RECOVER_SUCCESS) {
        ZLOGE("proxy recover success: %d", static_cast<int>(status));
        singleKvStore = std::make_shared<SingleKvStoreClient>(std::move(proxyTmp), storeIdTmp);
        return status;
    }

    if (status != Status::SUCCESS) {
        ZLOGE("proxy return error: %d", static_cast<int>(status));
        return status;
    }

    if (proxyTmp == nullptr) {
        ZLOGE("proxy return nullptr.");
        return status;
    }

    singleKvStore = std::make_shared<SingleKvStoreClient>(std::move(proxyTmp), storeIdTmp);
    return status;
}

Status DistributedKvDataManager::GetAllKvStoreId(const AppId &appId, std::vector<StoreId> &storeIds)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return Status::SERVER_UNAVAILABLE;
    }

    Status status;
    kvDataServiceProxy->GetAllKvStoreId(appId, [&status, &storeIds](auto statusTmp, auto &ids) {
        status = statusTmp;
        storeIds = std::move(ids);
    });
    return status;
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, const StoreId &storeId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    KvStoreServiceDeathNotifier::SetAppId(appId);
    std::string storeIdTmp = Constant::TrimCopy<std::string>(storeId.storeId);
    if (storeIdTmp.size() == 0 || storeIdTmp.size() > Constant::MAX_STORE_ID_LENGTH) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }

    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy != nullptr) {
        return kvDataServiceProxy->CloseKvStore(appId, storeId);
    }
    ZLOGE("proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status DistributedKvDataManager::CloseKvStore(const AppId &appId, std::shared_ptr<SingleKvStore> &kvStorePtr)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    if (kvStorePtr == nullptr) {
        ZLOGE("kvStorePtr is nullptr.");
        return Status::INVALID_ARGUMENT;
    }
    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy != nullptr) {
        StoreId storeId = kvStorePtr->GetStoreId();
        kvStorePtr = nullptr;
        return kvDataServiceProxy->CloseKvStore(appId, storeId);
    }
    ZLOGE("proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status DistributedKvDataManager::CloseAllKvStore(const AppId &appId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy != nullptr) {
        return kvDataServiceProxy->CloseAllKvStore(appId);
    }
    ZLOGE("proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status DistributedKvDataManager::DeleteKvStore(const AppId &appId, const StoreId &storeId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    std::string storeIdTmp = Constant::TrimCopy<std::string>(storeId.storeId);
    if (storeIdTmp.size() == 0 || storeIdTmp.size() > Constant::MAX_STORE_ID_LENGTH) {
        ZLOGE("invalid storeId.");
        return Status::INVALID_ARGUMENT;
    }

    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy != nullptr) {
        return kvDataServiceProxy->DeleteKvStore(appId, storeId);
    }
    ZLOGE("proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

Status DistributedKvDataManager::DeleteAllKvStore(const AppId &appId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::TRACE_CHAIN_ON);

    KvStoreServiceDeathNotifier::SetAppId(appId);
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy != nullptr) {
        return kvDataServiceProxy->DeleteAllKvStore(appId);
    }
    ZLOGE("proxy is nullptr.");
    return Status::SERVER_UNAVAILABLE;
}

void DistributedKvDataManager::RegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("Register KvStoreService Death Recipient input is null.");
        return;
    }
    KvStoreServiceDeathNotifier::AddServiceDeathWatcher(kvStoreDeathRecipient);
}

void DistributedKvDataManager::UnRegisterKvStoreServiceDeathRecipient(
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient)
{
    ZLOGD("begin");
    if (kvStoreDeathRecipient == nullptr) {
        ZLOGW("UnRegister KvStoreService Death Recipient input is null.");
        return;
    }
    KvStoreServiceDeathNotifier::RemoveServiceDeathWatcher(kvStoreDeathRecipient);
}

Status DistributedKvDataManager::GetLocalDevice(DeviceInfo &localDevice)
{
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return Status::ERROR;
    }

    return kvDataServiceProxy->GetLocalDevice(localDevice);
}

Status DistributedKvDataManager::GetDeviceList(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy)
{
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return Status::ERROR;
    }

    return kvDataServiceProxy->GetRemoteDevices(deviceInfoList, strategy);
}

static std::map<DeviceStatusChangeListener *, sptr<IDeviceStatusChangeListener>> deviceObservers_;
static std::mutex deviceObserversMapMutex_;
Status DistributedKvDataManager::StartWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    sptr<DeviceStatusChangeListenerClient> ipcObserver = new(std::nothrow) DeviceStatusChangeListenerClient(observer);
    if (ipcObserver == nullptr) {
        ZLOGW("new DeviceStatusChangeListenerClient failed");
        return Status::ERROR;
    }
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return Status::ERROR;
    }
    Status status = kvDataServiceProxy->StartWatchDeviceChange(ipcObserver, observer->GetFilterStrategy());
    if (status == Status::SUCCESS) {
        {
            std::lock_guard<std::mutex> lck(deviceObserversMapMutex_);
            deviceObservers_.insert({observer.get(), ipcObserver});
        }
        return Status::SUCCESS;
    }
    ZLOGE("watch failed.");
    return Status::ERROR;
}

Status DistributedKvDataManager::StopWatchDeviceChange(std::shared_ptr<DeviceStatusChangeListener> observer)
{
    sptr<IKvStoreDataService> kvDataServiceProxy = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (kvDataServiceProxy == nullptr) {
        ZLOGE("proxy is nullptr.");
        return Status::ERROR;
    }
    std::lock_guard<std::mutex> lck(deviceObserversMapMutex_);
    auto it = deviceObservers_.find(observer.get());
    if (it == deviceObservers_.end()) {
        ZLOGW(" not start watch device change.");
        return Status::ERROR;
    }
    Status status = kvDataServiceProxy->StopWatchDeviceChange(it->second);
    if (status == Status::SUCCESS) {
        deviceObservers_.erase(it->first);
    } else {
        ZLOGW("stop watch failed code=%d.", static_cast<int>(status));
    }
    return status;
}
}  // namespace DistributedKv
}  // namespace OHOS

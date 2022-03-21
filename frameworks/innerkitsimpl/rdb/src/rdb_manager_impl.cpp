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

#define LOG_TAG "RdbManagerImpl"

#include "rdb_manager_impl.h"

#include <thread>
#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "system_ability_definition.h"

#include "log_print.h"
#include "ikvstore_data_service.h"
#include "irdb_service.h"
#include "rdb_service_proxy.h"

namespace OHOS::DistributedRdb {
static sptr<DistributedKv::KvStoreDataServiceProxy> GetDistributedDataManager()
{
    int retry = 0;
    while (++retry <= RdbManagerImpl::GET_SA_RETRY_TIMES) {
        auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (manager == nullptr) {
            ZLOGE("get system ability manager failed");
            return nullptr;
        }
        ZLOGI("get distributed data manager %{public}d", retry);
        auto remoteObject = manager->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
        if (remoteObject == nullptr) {
            std::this_thread::sleep_for(std::chrono::seconds(RdbManagerImpl::RETRY_INTERVAL));
            continue;
        }
        ZLOGI("get distributed data manager success");
        return iface_cast<DistributedKv::KvStoreDataServiceProxy>(remoteObject);
    }

    ZLOGE("get distributed data manager failed");
    return nullptr;
}

static void LinkToDeath(const sptr<IRemoteObject>& remote)
{
    auto& manager = RdbManagerImpl::GetInstance();
    sptr<RdbManagerImpl::ServiceDeathRecipient> deathRecipient =
        new(std::nothrow) RdbManagerImpl::ServiceDeathRecipient(&manager);
    if (!remote->AddDeathRecipient(deathRecipient)) {
        ZLOGE("add death recipient failed");
    }
    ZLOGE("success");
}

RdbManagerImpl::RdbManagerImpl()
{
    ZLOGI("construct");
}

RdbManagerImpl::~RdbManagerImpl()
{
    ZLOGI("destroy");
}

RdbManagerImpl& RdbManagerImpl::GetInstance()
{
    static RdbManagerImpl manager;
    return manager;
}

sptr<IRdbService> RdbManagerImpl::GetRdbService()
{
    if (distributedDataMgr_ == nullptr) {
        distributedDataMgr_ = GetDistributedDataManager();
    }
    if (distributedDataMgr_ == nullptr) {
        ZLOGE("get distributed data manager failed");
        return nullptr;
    }

    auto service = distributedDataMgr_->GetRdbService();
    if (service == nullptr) {
        ZLOGE("get rdb service failed");
        return nullptr;
    }
    return service;
}

std::shared_ptr<RdbService> RdbManagerImpl::GetRdbService(const RdbSyncerParam& param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rdbService_ != nullptr) {
        return rdbService_;
    }
    auto service = GetRdbService();
    if (service == nullptr) {
        return nullptr;
    }
    if (((RdbServiceProxy *)service.GetRefPtr())->InitNotifier(param) != RDB_OK) {
        ZLOGE("init notifier failed");
        return nullptr;
    }
    LinkToDeath(service->AsObject());
    rdbService_ = std::shared_ptr<RdbService>(service.GetRefPtr(), [holder = service] (const auto*) {});
    bundleName_ = param.bundleName_;
    return rdbService_;
}

void RdbManagerImpl::OnRemoteDied()
{
    ZLOGI("rdb service has dead!!");
    auto proxy = std::static_pointer_cast<RdbServiceProxy>(rdbService_);
    auto observers = proxy->ExportObservers();
    ResetServiceHandle();

    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
    RdbSyncerParam param;
    param.bundleName_ = bundleName_;
    auto service = GetRdbService(param);
    if (service == nullptr) {
        return;
    }
    proxy = std::static_pointer_cast<RdbServiceProxy>(service);
    if (proxy == nullptr) {
        return;
    }
    ZLOGI("restore observer");
    proxy->ImportObservers(observers);
}

void RdbManagerImpl::ResetServiceHandle()
{
    ZLOGI("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    distributedDataMgr_ = nullptr;
    rdbService_ = nullptr;
}
} // namespace OHOS::DistributedRdb

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

#include "iservice_registry.h"
#include "ipc_skeleton.h"
#include "system_ability_definition.h"

#include "log_print.h"
#include "ikvstore_data_service.h"
#include "irdb_service.h"

using namespace OHOS::DistributedKv;
namespace OHOS::DistributedRdb {
class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit ServiceDeathRecipient(RdbManagerImpl* owner) : owner_(owner) {}
    void OnRemoteDied(const wptr<IRemoteObject> &object) override
    {
        if (owner_ != nullptr) {
            owner_->OnRemoteDied();
        }
    }
private:
    RdbManagerImpl* owner_;
};

static sptr<IKvStoreDataService> GetDistributedDataManager()
{
    auto manager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (manager == nullptr) {
        ZLOGE("get system ability manager failed");
        return nullptr;
    }
    ZLOGI("get distributed data manager");
    auto remoteObject = manager->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    return iface_cast<IKvStoreDataService>(remoteObject);
}

static void LinkToDeath(const sptr<IRemoteObject>& remote)
{
    auto& manager = RdbManagerImpl::GetInstance();
    sptr<ServiceDeathRecipient> deathRecipient = new(std::nothrow) ServiceDeathRecipient(&manager);
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
    ZLOGI("deconstruct");
}

RdbManagerImpl& RdbManagerImpl::GetInstance()
{
    static RdbManagerImpl manager;
    return manager;
}

std::shared_ptr<RdbService> RdbManagerImpl::GetRdbService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rdbService_ != nullptr) {
        return rdbService_;
    }
    
    if (distributedDataMgr_ == nullptr) {
        distributedDataMgr_ = GetDistributedDataManager();
    }
    if (distributedDataMgr_ == nullptr) {
        ZLOGE("get distributed data manager failed");
        return nullptr;
    }
    
    auto serviceObject = distributedDataMgr_->GetRdbService();
    if (serviceObject == nullptr) {
        ZLOGE("get rdb service failed");
        return nullptr;
    }
    LinkToDeath(serviceObject->AsObject());
    rdbService_ = std::shared_ptr<RdbService>(serviceObject.GetRefPtr(), [holder = serviceObject] (const auto*) {});
    return rdbService_;
}

std::shared_ptr<RdbSyncer> RdbManagerImpl::GetRdbSyncer(const RdbSyncerParam &param)
{
    if (param.bundleName_.empty() || param.path_.empty() || param.storeName_.empty()) {
        ZLOGE("param is invalid");
        return nullptr;
    }
    auto service = GetRdbService();
    if (service == nullptr) {
        return nullptr;
    }
    RegisterClientDeathRecipient(param.bundleName_);
    return service->GetRdbSyncer(param);
}

int RdbManagerImpl::RegisterRdbServiceDeathObserver(const std::string& storeName,
                                                    const std::function<void()>& observer)
{
    serviceDeathObservers_.Insert(storeName, observer);
    return 0;
}

int RdbManagerImpl::UnRegisterRdbServiceDeathObserver(const std::string& storeName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    serviceDeathObservers_.Erase(storeName);
    return 0;
}

void RdbManagerImpl::OnRemoteDied()
{
    ZLOGI("rdb service has dead!!");
    NotifyServiceDeath();
    ResetServiceHandle();
}

void RdbManagerImpl::ResetServiceHandle()
{
    ZLOGI("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    distributedDataMgr_ = nullptr;
    rdbService_ = nullptr;
    clientDeathObject_ = nullptr;
}

void RdbManagerImpl::NotifyServiceDeath()
{
    ZLOGI("enter");
    serviceDeathObservers_.ForEach([] (const auto& key, const auto& value) {
        if (value != nullptr) {
            value();
        }
        return false;
    });
}

void RdbManagerImpl::RegisterClientDeathRecipient(const std::string& bundleName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (clientDeathObject_ != nullptr) {
        return;
    }
    if (rdbService_ != nullptr) {
        sptr<IRemoteObject> object = new(std::nothrow) RdbClientDeathRecipientStub();
        if (rdbService_->RegisterClientDeathRecipient(bundleName, object) != 0) {
            ZLOGE("register client death recipient failed");
        } else {
            clientDeathObject_ = object;
        }
    }
}
}

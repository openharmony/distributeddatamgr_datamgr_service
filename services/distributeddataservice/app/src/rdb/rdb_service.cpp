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

#define LOG_TAG "RdbService"

#include "rdb_service.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "rdb_store.h"
#include "rdb_store_factory.h"

namespace OHOS::DistributedKv {
static bool operator==(const sptr<IRdbStore>& store, const std::string& appId)
{
    auto* storePtr = (RdbStore*)store.GetRefPtr();
    return storePtr != nullptr && (storePtr->GetAppId() == appId);
}

void RdbService::Initialzie()
{
    RdbStoreFactory::Initialize();
}

RdbService::ClientDeathRecipient::ClientDeathRecipient(DeathCallback& callback)
    : callback_(callback)
{
    ZLOGI("construct");
}

RdbService::ClientDeathRecipient::~ClientDeathRecipient()
{
    ZLOGI("deconstruct");
}

void RdbService::ClientDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    auto objectSptr = object.promote();
    if (objectSptr != nullptr && callback_ != nullptr) {
        callback_(objectSptr);
    }
}

void RdbService::ClearClientRecipient(const std::string& bundleName, sptr<IRemoteObject>& proxy)
{
    std::lock_guard<std::mutex> lock(recipientsLock_);
    ZLOGI("remove %{public}s", bundleName.c_str());
    recipients_.erase(proxy);
}

void RdbService::ClearClientStores(const std::string& bundleName)
{
    ZLOGI("enter");
    auto appId = KvStoreUtils::GetAppIdByBundleName(bundleName);
    std::lock_guard<std::mutex> lock(storesLock_);
    for (auto it = stores_.begin(); it != stores_.end();) {
        if (it->second == appId) {
            ZLOGI("remove %{public}s", it->first.c_str());
            stores_.erase(it++);
        } else {
            it++;
        }
    }
}

void RdbService::OnClientDied(const std::string &bundleName, sptr<IRemoteObject>& proxy)
{
    ZLOGI("%{public}s died", bundleName.c_str());
    ClearClientRecipient(bundleName, proxy);
    ClearClientStores(bundleName);
}

bool RdbService::CheckAccess(const RdbStoreParam& param) const
{
    return !KvStoreUtils::GetAppIdByBundleName(param.bundleName_).empty();
}

sptr<IRdbStore> RdbService::CreateStore(const RdbStoreParam& param)
{
    sptr<RdbStore> store = RdbStoreFactory::CreateStore(param);
    if (store == nullptr) {
        ZLOGE("create temp store failed");
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(storesLock_);
    for (const auto& entry : stores_) {
        if (*store == *entry.second) {
            ZLOGI("find %{public}s", store->GetStoreId().c_str());
            return entry.second;
        }
    }
    
    ZLOGI("create new store %{public}s", param.storeName_.c_str());
    if (store->Init() != 0) {
        ZLOGE("store init failed");
        return nullptr;
    }
    stores_.insert({store->GetIdentifier(), store});
    return store;
}

sptr<IRdbStore> RdbService::GetRdbStore(const RdbStoreParam& param)
{
    ZLOGI("%{public}s %{public}s %{public}s", param.bundleName_.c_str(),
          param.path_.c_str(), param.storeName_.c_str());
    if (!CheckAccess(param)) {
        ZLOGI("check access failed");
        return nullptr;
    }
    return CreateStore(param);
}

int RdbService::RegisterClientDeathRecipient(const std::string& bundleName, sptr<IRemoteObject> proxy)
{
    if (proxy == nullptr) {
        ZLOGE("recipient is nullptr");
        return -1;
    }
    
    std::lock_guard<std::mutex> lock(recipientsLock_);
    ClientDeathRecipient::DeathCallback callback = [bundleName, this] (sptr<IRemoteObject>& object) {
        OnClientDied(bundleName, object);
    };
    sptr<ClientDeathRecipient> recipient = new(std::nothrow) ClientDeathRecipient(callback);
    if (recipient == nullptr) {
        ZLOGE("malloc failed");
        return -1;
    }
    if (!proxy->AddDeathRecipient(recipient)) {
        ZLOGE("add death recipient failed");
        return -1;
    }
    auto it = recipients_.insert({proxy, recipient});
    if (!it.second) {
        ZLOGE("insert failed");
        return -1;
    }
    ZLOGI("success");
    return 0;
}
}

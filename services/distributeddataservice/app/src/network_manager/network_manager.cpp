/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "NetWorkManager"
#include "network_manager.h"

#include "i_net_conn_callback.h"
#include "log_print.h"
#include "net_conn_client.h"

using namespace OHOS::NetManagerStandard;
namespace OHOS::DistributedKv {
bool NetWorkManager::RegOnNetworkChange(KvStoreDataService *kvStoreDataService)
{
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(kvStoreDataService);
    if (observer == nullptr) {
        ZLOGE("new operator error.observer is nullptr");
        return false;
    }
    int nRet = DelayedSingleton<NetConnClient>::GetInstance()->RegisterNetConnCallback(observer);
    if (nRet != NETMANAGER_SUCCESS) {
        ZLOGE("RegisterNetConnCallback failed, ret = %{public}d", nRet);
        return false;
    }
    return true;
}

NetWorkManager &NetWorkManager::GetInstance()
{
    static NetWorkManager instance;
    return instance;
}

NetWorkManager::NetWorkManager() = default;

NetWorkManager::~NetWorkManager() = default;

int32_t NetWorkManager::NetConnCallbackObserver::NetAvailable(sptr<NetManagerStandard::NetHandle> &netHandle)
{
    ZLOGI("OnNetworkAvailable");
    return kvStoreDataService_->OnNetworkOnline();
}

int32_t NetWorkManager::NetConnCallbackObserver::NetUnavailable()
{
    ZLOGI("OnNetworkUnavailable");
    return kvStoreDataService_->OnNetworkOffline();
}

int32_t NetWorkManager::NetConnCallbackObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetAllCapabilities> &netAllCap)
{
    ZLOGI("OnNetCapabilitiesChange");
    return SUCCESS;
}

int32_t NetWorkManager::NetConnCallbackObserver::NetConnectionPropertiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetLinkInfo> &info)
{
    ZLOGI("OnNetConnectionPropertiesChange");
    return SUCCESS;
}

int32_t NetWorkManager::NetConnCallbackObserver::NetLost(sptr<NetHandle> &netHandle)
{
    ZLOGI("OnNetLost");
    return SUCCESS;
}

int32_t NetWorkManager::NetConnCallbackObserver::NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked)
{
    ZLOGI("OnNetBlockStatusChange");
    return SUCCESS;
}
} // namespace OHOS::DistributedKv

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

#define LOG_TAG "RdbServiceImpl"

#include "rdb_service_impl.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "rdb_syncer_impl.h"
#include "rdb_syncer_factory.h"

namespace OHOS::DistributedRdb {
RdbServiceImpl::ClientDeathRecipient::ClientDeathRecipient(DeathCallback& callback)
    : callback_(callback)
{
    ZLOGI("construct");
}

RdbServiceImpl::ClientDeathRecipient::~ClientDeathRecipient()
{
    ZLOGI("deconstruct");
}

void RdbServiceImpl::ClientDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& object)
{
    auto objectSptr = object.promote();
    if (objectSptr != nullptr && callback_ != nullptr) {
        callback_(objectSptr);
    }
}

void RdbServiceImpl::ClearClientRecipient(const std::string& bundleName, sptr<IRemoteObject>& proxy)
{
    ZLOGI("remove %{public}s", bundleName.c_str());
    recipients_.Erase(proxy);
}

void RdbServiceImpl::ClearClientSyncers(const std::string& bundleName)
{
    ZLOGI("enter");
    std::string appId = DistributedKv::KvStoreUtils::GetAppIdByBundleName(bundleName);
    auto count = syncers_.RemoveIf([&appId] (const std::string& key, const sptr<RdbSyncerImpl>& value) {
        return value->GetAppId() == appId;
    });
    ZLOGI("remove %{public}d", static_cast<int>(count));
}

void RdbServiceImpl::OnClientDied(const std::string &bundleName, sptr<IRemoteObject>& proxy)
{
    ZLOGI("%{public}s died", bundleName.c_str());
    ClearClientRecipient(bundleName, proxy);
    ClearClientSyncers(bundleName);
}

bool RdbServiceImpl::CheckAccess(const RdbSyncerParam& param) const
{
    return !DistributedKv::KvStoreUtils::GetAppIdByBundleName(param.bundleName_).empty();
}

sptr<IRdbSyncer> RdbServiceImpl::CreateSyncer(const RdbSyncerParam& param)
{
    RdbSyncerImpl* syncerNew = RdbSyncerFactory::GetInstance().CreateSyncer(param);
    auto syncer = syncers_.ComputeIfAbsent(syncerNew->GetIdentifier(),
        [&syncerNew] (const std::string& key) {
            ZLOGI("create new syncer %{public}s", key.c_str());
            return sptr<RdbSyncerImpl>(syncerNew);
        });
    if (syncer != nullptr) {
        syncer->Init();
    }
    return syncer;
}

sptr<IRdbSyncer> RdbServiceImpl::GetRdbSyncerInner(const RdbSyncerParam& param)
{
    ZLOGI("%{public}s %{public}s %{public}s", param.bundleName_.c_str(),
          param.path_.c_str(), param.storeName_.c_str());
    if (!CheckAccess(param)) {
        ZLOGI("check access failed");
        return nullptr;
    }
    return CreateSyncer(param);
}

int RdbServiceImpl::RegisterClientDeathRecipient(const std::string& bundleName, sptr<IRemoteObject> proxy)
{
    if (proxy == nullptr) {
        ZLOGE("recipient is nullptr");
        return -1;
    }
    
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
    if (!recipients_.Insert(proxy, recipient)) {
        ZLOGE("insert failed");
        return -1;
    }
    ZLOGI("success");
    return 0;
}
}

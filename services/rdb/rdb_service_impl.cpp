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
#include "checker/checker_manager.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "rdb_syncer_impl.h"
#include "rdb_syncer_factory.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
RdbServiceImpl::ClientDeathRecipient::ClientDeathRecipient(std::function<void(sptr<IRemoteObject>&)> callback)
    : callback_(std::move(callback))
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

void RdbServiceImpl::ClearClientRecipient(sptr<IRemoteObject>& proxy)
{
    recipients_.Erase(proxy);
}

void RdbServiceImpl::ClearClientSyncers(pid_t pid)
{
    ZLOGI("enter");
    auto count = syncers_.Erase(pid);
    ZLOGI("remove %{public}d", static_cast<int>(count));
}

void RdbServiceImpl::OnClientDied(pid_t pid, sptr<IRemoteObject>& proxy)
{
    ClearClientRecipient(proxy);
    ClearClientSyncers(pid);
}

sptr<IRdbSyncer> RdbServiceImpl::CreateSyncer(const RdbSyncerParam& param, pid_t uid, pid_t pid)
{
    sptr<RdbSyncerImpl> syncer;
    syncers_.Compute(pid, [&param, uid, &syncer] (const auto &key, auto &syncers) -> bool {
        ZLOGI("create new syncer %{public}d", key);
        auto it = syncers.find(param.storeName_);
        if (it != syncers.end()) {
            syncer = it->second;
            return true;
        }
        syncer = RdbSyncerFactory::GetInstance().CreateSyncer(param, uid);
        if (syncer == nullptr) {
            return false;
        }
        syncer->Init();
        syncers.insert({param.storeName_, syncer});
        return true;
    });
    return syncer;
}

sptr<IRdbSyncer> RdbServiceImpl::GetRdbSyncerInner(const RdbSyncerParam& param)
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    pid_t pid = IPCSkeleton::GetCallingPid();
    ZLOGI("%{public}s %{public}s %{public}s", param.bundleName_.c_str(),
          param.path_.c_str(), param.storeName_.c_str());
    if (!CheckerManager::GetInstance().IsValid(param.bundleName_, uid)) {
        ZLOGI("check access failed");
        return nullptr;
    }
    return CreateSyncer(param, uid, pid);
}

int RdbServiceImpl::RegisterClientDeathRecipient(const std::string& bundleName, sptr<IRemoteObject> proxy)
{
    if (proxy == nullptr) {
        ZLOGE("recipient is nullptr");
        return -1;
    }
    
    pid_t pid = IPCSkeleton::GetCallingPid();
    sptr<ClientDeathRecipient> recipient = new(std::nothrow) ClientDeathRecipient(
        [this, pid](sptr<IRemoteObject>& object) {
            OnClientDied(pid, object);
        });
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

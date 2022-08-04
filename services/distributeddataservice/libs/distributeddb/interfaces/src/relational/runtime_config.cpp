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
#ifdef RELATIONAL_STORE
#include "runtime_config.h"

#include "db_constant.h"
#include "db_dfx_adapter.h"
#include "kvdb_manager.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "network_adapter.h"
#include "runtime_context.h"

namespace DistributedDB {
std::mutex RuntimeConfig::communicatorMutex_;
std::mutex RuntimeConfig::multiUserMutex_;
std::shared_ptr<IProcessCommunicator> RuntimeConfig::processCommunicator_ = nullptr;

// Used to set the process userid and appId
DBStatus RuntimeConfig::SetProcessLabel(const std::string &appId, const std::string &userId)
{
    if (appId.size() > DBConstant::MAX_APP_ID_LENGTH || appId.empty() ||
        userId.size() > DBConstant::MAX_USER_ID_LENGTH || userId.empty()) {
        LOGE("Invalid app or user info[%zu]-[%zu]", appId.length(), userId.length());
        return INVALID_ARGS;
    }

    int errCode = KvDBManager::SetProcessLabel(appId, userId);
    if (errCode != E_OK) {
        LOGE("Failed to set the process label:%d", errCode);
        return DB_ERROR;
    }
    return OK;
}

// Set process communicator.
DBStatus RuntimeConfig::SetProcessCommunicator(const std::shared_ptr<IProcessCommunicator> &inCommunicator)
{
    std::lock_guard<std::mutex> lock(communicatorMutex_);
    if (processCommunicator_ != nullptr) {
        LOGE("processCommunicator_ is not null!");
        return DB_ERROR;
    }

    std::string processLabel = RuntimeContext::GetInstance()->GetProcessLabel();
    if (processLabel.empty()) {
        LOGE("ProcessLabel is not set!");
        return DB_ERROR;
    }

    auto *adapter = new (std::nothrow) NetworkAdapter(processLabel, inCommunicator);
    if (adapter == nullptr) {
        LOGE("New NetworkAdapter failed!");
        return DB_ERROR;
    }
    processCommunicator_ = inCommunicator;
    if (RuntimeContext::GetInstance()->SetCommunicatorAdapter(adapter) != E_OK) {
        LOGE("SetProcessCommunicator not support!");
        delete adapter;
        return DB_ERROR;
    }
    KvDBManager::RestoreSyncableKvStore();
    return OK;
}

DBStatus RuntimeConfig::SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback)
{
    int errCode = RuntimeContext::GetInstance()->SetPermissionCheckCallback(callback);
    return TransferDBErrno(errCode);
}

DBStatus RuntimeConfig::SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback)
{
    int errCode = RuntimeContext::GetInstance()->SetPermissionCheckCallback(callback);
    return TransferDBErrno(errCode);
}

DBStatus RuntimeConfig::SetProcessSystemAPIAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter)
{
    return TransferDBErrno(RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter));
}

void RuntimeConfig::Dump(int fd, const std::vector<std::u16string> &args)
{
    DBDfxAdapter::Dump(fd, args);
}

DBStatus RuntimeConfig::SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback)
{
    int errCode = RuntimeContext::GetInstance()->SetSyncActivationCheckCallback(callback);
    return TransferDBErrno(errCode);
}

DBStatus RuntimeConfig::NotifyUserChanged()
{
    std::lock_guard<std::mutex> lock(multiUserMutex_);
    int errCode = RuntimeContext::GetInstance()->NotifyUserChanged();
    return TransferDBErrno(errCode);
}

bool RuntimeConfig::IsProcessSystemApiAdapterValid()
{
    return RuntimeContext::GetInstance()->IsProcessSystemApiAdapterValid();
}

DBStatus RuntimeConfig::SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback)
{
    int errCode = RuntimeContext::GetInstance()->SetSyncActivationCheckCallback(callback);
    return TransferDBErrno(errCode);
}

DBStatus RuntimeConfig::SetPermissionConditionCallback(const PermissionConditionCallback &callback)
{
    int errCode = RuntimeContext::GetInstance()->SetPermissionConditionCallback(callback);
    return TransferDBErrno(errCode);
}
} // namespace DistributedDB
#endif
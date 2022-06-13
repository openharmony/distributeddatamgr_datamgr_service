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
#define LOG_TAG "KVDBServiceImpl"
#include "kvdb_service_impl.h"

#include "account/account_delegate.h"
#include "communication_provider.h"
#include "ipc_skeleton.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;
KVDBServiceImpl::KVDBServiceImpl()
{
}

KVDBServiceImpl::~KVDBServiceImpl()
{
}

Status KVDBServiceImpl::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    std::vector<StoreMetaData> metaData;
    auto user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(IPCSkeleton::GetCallingPid());
    auto deviceId = CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    auto prefix = StoreMetaData::GetPrefix({ deviceId, user, "default", appId.appId });
    MetaDataManager::GetInstance().LoadMeta(prefix, metaData);
    for (auto &item : metaData) {
        storeIds.push_back({ item.storeId });
    }
    return SUCCESS;
}

Status KVDBServiceImpl::Delete(const AppId &appId, const StoreId &storeId)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::RegisterSyncCallback(
    const AppId &appId, const StoreId &storeId, sptr<IKvStoreSyncCallback> callback)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::UnregisterSyncCallback(const AppId &appId, const StoreId &storeId)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::EnableCapability(const AppId &appId, const StoreId &storeId)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::DisableCapability(const AppId &appId, const StoreId &storeId)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::SetCapability(const AppId &appId, const StoreId &storeId,
    const std::vector<std::string> &local, const std::vector<std::string> &remote)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::AddSubscribeInfo(
    const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices, const std::string &query)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::RmvSubscribeInfo(
    const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices, const std::string &query)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return SUCCESS;
}

Status KVDBServiceImpl::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    return SUCCESS;
}

Status KVDBServiceImpl::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    return NOT_SUPPORT;
}

Status KVDBServiceImpl::AfterCreate(
    const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password)
{
    return NOT_SUPPORT;
}
} // namespace OHOS::DistributedKv
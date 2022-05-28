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
#define LOG_TAG "KVDBServiceClient"
#include "kvdb_service_client.h"

#include "../../distributeddatafwk/src/kvstore_service_death_notifier.h"
#include "itypes_util.h"
#include "log_print.h"
#include "security_manager.h"
#include "single_store_impl.h"
#include "store_factory.h"
namespace OHOS::DistributedKv {
#define IPC_SEND(code, ...)                                                     \
    ({                                                                          \
        int32_t __status;                                                       \
        do {                                                                    \
            MessageParcel reply;                                                \
            MessageParcel request;                                              \
            if (!request.WriteInterfaceToken(GetDescriptor())) {                \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            if (!ITypesUtil::Marshal(request, ##__VA_ARGS__)) {                 \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            MessageOption option;                                               \
            auto result = remote_->SendRequest((code), request, reply, option); \
            if (result != 0) {                                                  \
                __status = IPC_ERROR;                                           \
                break;                                                          \
            }                                                                   \
                                                                                \
            ITypesUtil::Unmarshal(reply, __status);                             \
        } while (0);                                                            \
        __status;                                                               \
    })

BrokerDelegator<KVDBServiceClient> KVDBServiceClient::delegator_;
std::shared_ptr<KVDBServiceClient> KVDBServiceClient::CreateInstance()
{
    sptr<IKvStoreDataService> service = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    sptr<KVDBServiceClient> proxy = iface_cast<KVDBServiceClient>(service->GetKVdbService());
    return std::shared_ptr<KVDBServiceClient>(
        proxy.GetRefPtr(), [proxy](KVDBServiceClient *) mutable { proxy = nullptr; });
}

KVDBServiceClient::KVDBServiceClient(const OHOS::sptr<OHOS::IRemoteObject> &handle) : IRemoteProxy(handle)
{
    remote_ = Remote();
}

Status KVDBServiceClient::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    int32_t status = IPC_SEND(TRANS_GET_STORE_IDS, appId, storeIds);
    if (status != SUCCESS) {
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::BeforeCreate(const Options &options, const AppId &appId, const StoreId &storeId)
{
    int32_t status = IPC_SEND(TRANS_BEFORE_CREATE, appId, storeId);
    if (status != SUCCESS) {
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::AfterCreate(
    const Options &options, const AppId &appId, const StoreId &storeId, const std::vector<uint8_t> &password)
{
    int32_t status = IPC_SEND(TRANS_AFTER_CREATE, options, appId, storeId, password);
    if (status != SUCCESS) {
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::Delete(const std::string &path, const AppId &appId, const StoreId &storeId)
{
    int32_t status = IPC_SEND(TRANS_DELETE, path, appId, storeId);
    if (status != SUCCESS) {
    }
    return StoreFactory::GetInstance().Delete(path, appId, storeId);
}

Status KVDBServiceClient::Sync(const std::vector<std::string> &devices, const AppId &appId, const StoreId &storeId)
{
    int32_t status = IPC_SEND(TRANS_SYNC, devices, appId, storeId);
    if (status != SUCCESS) {
        ZLOGE("failed, status:%{public}d, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

std::shared_ptr<SingleKvStore> KVDBServiceClient::GetKVStore(
    const std::string &path, const Options &options, const AppId &appId, const StoreId &storeId, Status &status)
{
    bool isExits = StoreFactory::GetInstance().IsExits(appId, storeId);
    if (isExits) {
        return StoreFactory::GetInstance().Create(path, options, appId, storeId, status);
    }
    status = BeforeCreate(options, appId, storeId);
    auto kvStore = StoreFactory::GetInstance().Create(path, options, appId, storeId, status);
    auto password = SecurityManager::GetInstance().GetDBPassword(path, appId, storeId);
    std::vector<uint8_t> pwd(password.GetData(), password.GetData() + password.GetSize());
    status = AfterCreate(options, appId, storeId, pwd);
    pwd.assign(pwd.size(), 0);
    return kvStore;
}

Status KVDBServiceClient::CloseKVStore(const AppId &appId, const StoreId &storeId)
{
    return StoreFactory::GetInstance().Close(appId, storeId);
}

Status KVDBServiceClient::CloseKVStore(const AppId &appId, std::shared_ptr<SingleKVStore> &kvStore)
{
    auto status = StoreFactory::GetInstance().Close(appId, { kvStore->GetStoreId() });
    kvStore = nullptr;
    return status;
}

Status KVDBServiceClient::CloseAllKVStore(const AppId &appId)
{
    return StoreFactory::GetInstance().Close(appId, { "" });
}
} // namespace OHOS::DistributedKv
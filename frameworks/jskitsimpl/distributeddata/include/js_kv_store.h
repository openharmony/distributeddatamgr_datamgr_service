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
#ifndef OHOS_KV_STORE_H
#define OHOS_KV_STORE_H
#include "napi_queue.h"
#include "single_kvstore.h"
#include "uv_queue.h"

namespace OHOS::DistributedData {
enum {
    /* exported js SubscribeType  is (DistributedKv::SubscribeType-1) */
    SUBSCRIBE_LOCAL = 0,        /* i.e. SubscribeType::SUBSCRIBE_TYPE_LOCAL-1  */
    SUBSCRIBE_REMOTE = 1,       /* i.e. SubscribeType::SUBSCRIBE_TYPE_REMOTE-1 */
    SUBSCRIBE_LOCAL_REMOTE = 2, /* i.e. SubscribeType::SUBSCRIBE_TYPE_ALL--1   */
    SUBSCRIBE_COUNT = 3
};

/* [NOTES]
 *    OHOS::DistributedData::JsKVStore is NOT related to DistributedKv::KvStore!!!
 *    OHOS::DistributedData::JsKVStore is wrapped for DistributedKv::SingleKvStore...
 */
class JsKVStore {
public:
    JsKVStore(const std::string& storeId);
    virtual ~JsKVStore();

    void SetNative(std::shared_ptr<DistributedKv::SingleKvStore>& kvStore);
    std::shared_ptr<DistributedKv::SingleKvStore>& GetNative();

    static bool IsInstanceOf(napi_env env, napi_value obj, const std::string& storeId, napi_value constructor);

    /* public static members */
    static napi_value Put(napi_env env, napi_callback_info info);
    static napi_value Delete(napi_env env, napi_callback_info info);
    static napi_value OnEvent(napi_env env, napi_callback_info info);
    static napi_value OffEvent(napi_env env, napi_callback_info info);
    static napi_value PutBatch(napi_env env, napi_callback_info info);
    static napi_value DeleteBatch(napi_env env, napi_callback_info info);
    static napi_value StartTransaction(napi_env env, napi_callback_info info);
    static napi_value Commit(napi_env env, napi_callback_info info);
    static napi_value Rollback(napi_env env, napi_callback_info info);
    static napi_value EnableSync(napi_env env, napi_callback_info info);
    static napi_value SetSyncRange(napi_env env, napi_callback_info info);

private:
    /* private static members */
    static void OnDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt);
    static void OffDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt);

    static void OnSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt);
    static void OffSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt);

    /* private non-static members */
    napi_status Subscribe(uint8_t type, std::shared_ptr<DistributedKv::KvStoreObserver> observer);
    napi_status UnSubscribe(uint8_t type, std::shared_ptr<DistributedKv::KvStoreObserver> observer);

    napi_status RegisterSyncCallback(std::shared_ptr<DistributedKv::KvStoreSyncCallback> sync);
    napi_status UnRegisterSyncCallback();

    /* private non-static members */
    std::shared_ptr<DistributedKv::SingleKvStore> kvStore_ = nullptr;
    std::string storeId_;

    using Exec = std::function<void(napi_env, size_t, napi_value*, std::shared_ptr<ContextBase>)>;
    static std::map<std::string, Exec> onEventHandlers_;
    static std::map<std::string, Exec> offEventHandlers_;

    std::shared_ptr<DistributedKv::KvStoreSyncCallback> syncObserver_ = nullptr;
    std::mutex listMutex_ {};
    std::list<std::shared_ptr<DistributedKv::KvStoreObserver>> dataObserver_[SUBSCRIBE_COUNT];
};

class DataObserver : public DistributedKv::KvStoreObserver, public UvQueue {
public:
    DataObserver(napi_env env, napi_value callback);
    virtual ~DataObserver() = default;

    void OnChange(const DistributedKv::ChangeNotification& notification,
        std::shared_ptr<DistributedKv::KvStoreSnapshot> snapshot) override;
    void OnChange(const DistributedKv::ChangeNotification& notification) override;
};

class SyncObserver : public DistributedKv::KvStoreSyncCallback, public UvQueue {
public:
    SyncObserver(napi_env env, napi_value callback);
    virtual ~SyncObserver() = default;

    void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;
};
} // namespace OHOS::DistributedData
#endif // OHOS_SINGLE_KV_STORE_H

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
#define LOG_TAG "JS_KVStore"
#include "js_kv_store.h"
#include "js_util.h"
#include "js_kv_store_resultset.h"
#include "log_print.h"
#include "napi_queue.h"
#include "datashare_values_bucket.h"
#include "datashare_predicates.h"
#include "single_kvstore.h"
#include "kv_utils.h"
#include "kvstore_datashare_result_set.h"
#include "kvstore_predicates.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;

namespace OHOS::DistributedData {
std::map<std::string, JsKVStore::Exec> JsKVStore::onEventHandlers_ = {
    { "dataChange", JsKVStore::OnDataChange },
    { "syncComplete", JsKVStore::OnSyncComplete }
};

std::map<std::string, JsKVStore::Exec> JsKVStore::offEventHandlers_ = {
    { "dataChange", JsKVStore::OffDataChange },
    { "syncComplete", JsKVStore::OffSyncComplete }
};

static bool ValidSubscribeType(uint8_t type)
{
    return (SUBSCRIBE_LOCAL <= type) && (type <= SUBSCRIBE_LOCAL_REMOTE);
}

static SubscribeType ToSubscribeType(uint8_t type)
{
    return static_cast<SubscribeType>(type + 1);
}

JsKVStore::JsKVStore(const std::string& storeId)
    : storeId_(storeId)
{
}

JsKVStore::~JsKVStore()
{
    ZLOGD("no memory leak for JsKVStore");
    if (kvStore_ == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lck(listMutex_);
    for (uint8_t type = SUBSCRIBE_LOCAL; type < SUBSCRIBE_COUNT; type++) {
        for (auto& observer : dataObserver_[type]) {
            auto subscribeType = ToSubscribeType(type);
            kvStore_->UnSubscribeKvStore(subscribeType, observer);
            observer->Clear();
        }
        dataObserver_[type].clear();
    }

    kvStore_->UnRegisterSyncCallback();
    for (auto &syncObserver : syncObservers_) {
        syncObserver->Clear();
    }
    syncObservers_.clear();
}

void JsKVStore::SetNative(std::shared_ptr<SingleKvStore>& kvStore)
{
    kvStore_ = kvStore;
}

std::shared_ptr<SingleKvStore>& JsKVStore::GetNative()
{
    return kvStore_;
}

bool JsKVStore::IsInstanceOf(napi_env env, napi_value obj, const std::string& storeId, napi_value constructor)
{
    bool result = false;
    napi_status status = napi_instanceof(env, obj, constructor, &result);
    CHECK_RETURN((status == napi_ok) && (result != false), "is not instance of JsKVStore!", false);

    JsKVStore* kvStore = nullptr;
    status = napi_unwrap(env, obj, reinterpret_cast<void**>(&kvStore));
    CHECK_RETURN((status == napi_ok) && (kvStore != nullptr), "can not unwrap to JsKVStore!", false);
    return kvStore->storeId_ == storeId;
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      put(key:string, value:Uint8Array | string | boolean | number, callback: AsyncCallback<void>):void;
 * [Promise]
 *      put(key:string, value:Uint8Array | string | boolean | number):Promise<void>;
 */
napi_value JsKVStore::Put(napi_env env, napi_callback_info info)
{
    ZLOGD("KVStore::Put()");
    struct PutContext : public ContextBase {
        std::string key;
        std::vector<uint8_t> value;
        std::vector<DataShareValuesBucket> valueBuckets;
        std::vector<Entry> entries;
        napi_valuetype type;
    };

    auto ctxt = std::make_shared<PutContext>();

    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <key> <value>
        CHECK_ARGS_RETURN_VOID(ctxt, (argc == 1 | argc == 2), "invalid arguments!");
        ctxt->type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &(ctxt->type));
        if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->valueBuckets);
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid valueBuckets!");
        } else if (ctxt->type == napi_string) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->key);
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid key!");
            JSUtil::KvStoreVariant vv;
            ctxt->status = JSUtil::GetValue(env, argv[1], vv);
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid value!");
            DistributedKv::Blob blob = JSUtil::VariantValue2Blob(vv);
            ctxt->value = blob.Data();
        }
    });

    auto execute = [ctxt]() {
        Status status = Status::ERROR;
        if (ctxt->type == napi_object) {
            auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
            ctxt->entries = KvUtils::ToEntry(ctxt->valueBuckets);
            ZLOGD("kvStoreDataShare->ToEntry return");
            status = kvStore->PutBatch(ctxt->entries);
            CHECK_STATUS_RETURN_VOID(ctxt, "kvStoreDataShare->Put, i.e. Put error!");
        } else if (ctxt->type == napi_string) {
            OHOS::DistributedKv::Key key(ctxt->key);
            OHOS::DistributedKv::Value value(ctxt->value);
            auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
            status = kvStore->Put(key, value);
            ZLOGD("kvStore->Put return %{public}d", status);
        }
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->Put() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      delete(key: string, callback: AsyncCallback<void>): void;
 * [Promise]
 *      delete(key: string): Promise<void>;
 */
napi_value JsKVStore::Delete(napi_env env, napi_callback_info info)
{
    ZLOGD("KVStore::Delete()");
    struct DeleteContext : public ContextBase {
        std::string key;
        DataSharePredicates predicates;
        napi_valuetype type;
    };
    auto ctxt = std::make_shared<DeleteContext>();

    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <key> || <predicates>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &(ctxt->type));
        if (ctxt->type == napi_string) { 
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->key);
            ZLOGD("kvStore->Put return %{public}d", ctxt->status);
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid key!");
        } else if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->predicates);
            ZLOGD("kvStoreDataShare->Delete return %{public}d", ctxt->status);
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid predicates!");
        }
    });

    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), [ctxt]() {
        Status status = Status::ERROR;
        if (ctxt->type == napi_object) {
            std::vector<Key> keys;
            status = KvUtils::GetKeys(ctxt->predicates, keys);
            ZLOGD("GetKeys return %{public}d", status);
            auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
            status = kvStore->DeleteBatch(keys);
            ZLOGD("kv Datashare Delete return %{public}d", status);
        } else if (ctxt->type == napi_string) {
            OHOS::DistributedKv::Key key(ctxt->key);
            auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
            status = kvStore->Delete(key);
        }
        ZLOGD("kvStore->Delete return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->Delete() failed!");
    });
}

/*
 * [JS API Prototype]
 * [Callback]
 *      on(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
napi_value JsKVStore::OnEvent(napi_env env, napi_callback_info info)
{
    ZLOGD("in");
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <event> [...] <callback>
        CHECK_ARGS_RETURN_VOID(ctxt, argc >= 2, "invalid arguments!");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        ZLOGI("subscribe to event:%{public}s", event.c_str());
        auto handle = onEventHandlers_.find(event);
        CHECK_ARGS_RETURN_VOID(ctxt, handle != onEventHandlers_.end(), "invalid arg[0], i.e. unsupported event");
        // shift 1 argument, for JsKVStore::Exec.
        handle->second(env, argc - 1, &argv[1], ctxt);
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return nullptr;
}

/*
 * [JS API Prototype]
 * [Callback]
 *      off(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 *      off(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
napi_value JsKVStore::OffEvent(napi_env env, napi_callback_info info)
{
    ZLOGD("in");
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <event> [callback]
        CHECK_ARGS_RETURN_VOID(ctxt, argc >= 1, "invalid arguments!");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        ZLOGI("unsubscribe to event:%{public}s", event.c_str());
        auto handle = offEventHandlers_.find(event);
        CHECK_ARGS_RETURN_VOID(ctxt, handle != offEventHandlers_.end(), "invalid arg[0], i.e. unsupported event");
        // shift 1 argument, for JsKVStore::Exec.
        handle->second(env, argc - 1, &argv[1], ctxt);
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return nullptr;
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      putBatch(entries: Entry[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      putBatch(entries: Entry[]):Promise<void>;
 */
napi_value JsKVStore::PutBatch(napi_env env, napi_callback_info info)
{
    struct PutBatchContext : public ContextBase {
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<PutBatchContext>();

    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <entries>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->entries);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid entries!");
    });

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->PutBatch(ctxt->entries);
        ZLOGD("kvStore->DeleteBatch return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->PutBatch() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      deleteBatch(keys: string[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      deleteBatch(keys: string[]):Promise<void>;
 */
napi_value JsKVStore::DeleteBatch(napi_env env, napi_callback_info info)
{
    struct DeleteBatchContext : public ContextBase {
        std::vector<std::string> keys;
    };
    auto ctxt = std::make_shared<DeleteBatchContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <keys>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        JSUtil::GetValue(env, argv[0], ctxt->keys);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid keys!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        std::vector<DistributedKv::Key> keys;
        for (auto it : ctxt->keys) {
            DistributedKv::Key key(it);
            keys.push_back(key);
        }
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->DeleteBatch(keys);
        ZLOGD("kvStore->DeleteBatch return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->DeleteBatch failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      startTransaction(callback: AsyncCallback<void>):void;
 * [Promise]
 *      startTransaction() : Promise<void>;
 */
napi_value JsKVStore::StartTransaction(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->StartTransaction();
        ZLOGD("kvStore->StartTransaction return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->StartTransaction() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      commit(callback: AsyncCallback<void>):void;
 * [Promise]
 *      commit() : Promise<void>;
 */
napi_value JsKVStore::Commit(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->Commit();
        ZLOGD("kvStore->Commit return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->Commit() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      rollback(callback: AsyncCallback<void>):void;
 * [Promise]
 *      rollback() : Promise<void>;
 */
napi_value JsKVStore::Rollback(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->Rollback();
        ZLOGD("kvStore->Commit return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->Rollback() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      enableSync(enabled:boolean, callback: AsyncCallback<void>):void;
 * [Promise]
 *      enableSync(enabled:boolean) : Promise<void>;
 */
napi_value JsKVStore::EnableSync(napi_env env, napi_callback_info info)
{
    struct EnableSyncContext : public ContextBase {
        bool enable = false;
    };
    auto ctxt = std::make_shared<EnableSyncContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <enable>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = napi_get_value_bool(env, argv[0], &ctxt->enable);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid enabled!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->SetCapabilityEnabled(ctxt->enable);
        ZLOGD("kvStore->SetCapabilityEnabled return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->SetCapabilityEnabled() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      setSyncRange(localLabels:string[], remoteSupportLabels:string[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      setSyncRange(localLabels:string[], remoteSupportLabels:string[]) : Promise<void>;
 */
napi_value JsKVStore::SetSyncRange(napi_env env, napi_callback_info info)
{
    struct SyncRangeContext : public ContextBase {
        std::vector<std::string> localLabels;
        std::vector<std::string> remoteSupportLabels;
    };
    auto ctxt = std::make_shared<SyncRangeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <localLabels> <remoteSupportLabels>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->localLabels);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid localLabels!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->remoteSupportLabels);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid remoteSupportLabels!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->SetCapabilityRange(ctxt->localLabels, ctxt->remoteSupportLabels);
        ZLOGD("kvStore->SetCapabilityRange return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->SetCapabilityRange() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype] JsKVStore::OnDataChange is private non-static.
 * [Callback]
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
void JsKVStore::OnDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 2 arguments :: <SubscribeType> <observer>
    CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments on dataChange!");

    int32_t type = SUBSCRIBE_COUNT;
    ctxt->status = napi_get_value_int32(env, argv[0], &type);
    CHECK_STATUS_RETURN_VOID(ctxt, "napi_get_value_int32 failed!");
    CHECK_ARGS_RETURN_VOID(ctxt, ValidSubscribeType(type), "invalid arg[1], i.e. invalid subscribeType");

    napi_valuetype valueType = napi_undefined;
    ctxt->status = napi_typeof(env, argv[1], &valueType);
    CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
    CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "invalid arg[2], i.e. invalid callback");

    ZLOGI("subscribe data change type %{public}d", type);
    auto proxy = reinterpret_cast<JsKVStore*>(ctxt->native);
    std::lock_guard<std::mutex> lck(proxy->listMutex_);
    for (auto& it : proxy->dataObserver_[type]) {
        if (JSUtil::Equals(env, argv[1], it->GetCallback())) {
            ZLOGI("function is already subscribe type");
            return;
        }
    }

    ctxt->status = proxy->Subscribe(type, std::make_shared<DataObserver>(proxy->uvQueue_, argv[1]));
    CHECK_STATUS_RETURN_VOID(ctxt, "Subscribe failed!");
}

/*
 * [JS API Prototype] JsKVStore::OffDataChange is private non-static.
 * [Callback]
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 * [NOTES!!!]  no SubscribeType while off...
 *      off(event:'dataChange', observer: Callback<ChangeNotification>): void;
 */
void JsKVStore::OffDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: [callback]
    CHECK_ARGS_RETURN_VOID(ctxt, argc <= 1, "invalid arguments off dataChange!");
    // have 1 arguments :: have the callback
    if (argc == 1) {
        napi_valuetype valueType = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &valueType);
        CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
        CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "invalid arg[1], i.e. invalid callback");
    }
    ZLOGI("unsubscribe dataChange, %{public}s specified observer.", (argc == 0) ? "without": "with");

    auto proxy = reinterpret_cast<JsKVStore*>(ctxt->native);
    bool found = false;
    napi_status status = napi_ok;
    auto traverseType = [argc, argv, proxy, env, &found, &status](uint8_t type, auto& observers) {
        auto it = observers.begin();
        while (it != observers.end()) {
            if ((argc == 1) && !JSUtil::Equals(env, argv[0], (*it)->GetCallback())) {
                ++it;
                continue; // specified observer and not current iterator
            }
            found = true;
            status = proxy->UnSubscribe(type, *it);
            if (status != napi_ok) {
                break; // stop on fail.
            }
            it = observers.erase(it);
        }
    };

    std::lock_guard<std::mutex> lck(proxy->listMutex_);
    for (uint8_t type = SUBSCRIBE_LOCAL; type < SUBSCRIBE_COUNT; type++) {
        traverseType(type, proxy->dataObserver_[type]);
        if (status != napi_ok) {
            break; // stop on fail.
        }
    }
    CHECK_ARGS_RETURN_VOID(ctxt, found || (argc == 0), "not Subscribed!");
}

/*
 * [JS API Prototype] JsKVStore::OnSyncComplete is private non-static.
 * [Callback]
 *      on(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 */
void JsKVStore::OnSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: <callback>
    CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments on syncComplete!");
    napi_valuetype valueType = napi_undefined;
    ctxt->status = napi_typeof(env, argv[0], &valueType);
    CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
    CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "invalid arg[1], i.e. invalid callback");

    auto proxy = reinterpret_cast<JsKVStore*>(ctxt->native);
    ctxt->status = proxy->RegisterSyncCallback(std::make_shared<SyncObserver>(proxy->uvQueue_, argv[0]));
    CHECK_STATUS_RETURN_VOID(ctxt, "RegisterSyncCallback failed!");
}

/*
 * [JS API Prototype] JsKVStore::OffSyncComplete is private non-static.
 * [Callback]
 *      off(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 */
void JsKVStore::OffSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: [callback]
    CHECK_ARGS_RETURN_VOID(ctxt, argc <= 1, "invalid arguments off syncComplete!");
    auto proxy = reinterpret_cast<JsKVStore*>(ctxt->native);
    // have 1 arguments :: have the callback
    if (argc == 1) {
        napi_valuetype valueType = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &valueType);
        CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
        CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "invalid arg[1], i.e. invalid callback");
        std::lock_guard<std::mutex> lck(proxy->listMutex_);
        auto it = proxy->syncObservers_.begin();
        while (it != proxy->syncObservers_.end()) {
            if (JSUtil::Equals(env, argv[0], (*it)->GetCallback())) {
                (*it)->Clear();
                proxy->syncObservers_.erase(it);
                break;
            }
        }
        ctxt->status = napi_ok;
    }
    ZLOGI("unsubscribe syncComplete, %{public}s specified observer.", (argc == 0) ? "without": "with");
    if (argc == 0 || proxy->syncObservers_.empty()) {
        ctxt->status = proxy->UnRegisterSyncCallback();
    }
    CHECK_STATUS_RETURN_VOID(ctxt, "UnRegisterSyncCallback failed!");
}

/*
 * [Internal private non-static]
 */
napi_status JsKVStore::RegisterSyncCallback(std::shared_ptr<SyncObserver> callback)
{
    Status status = kvStore_->RegisterSyncCallback(callback);
    if (status != Status::SUCCESS) {
        callback->Clear();
        return napi_generic_failure;
    }
    std::lock_guard<std::mutex> lck(listMutex_);
    syncObservers_.push_back(callback);
    return napi_ok;
}

napi_status JsKVStore::UnRegisterSyncCallback()
{
    Status status = kvStore_->UnRegisterSyncCallback();
    if (status != Status::SUCCESS) {
        return napi_generic_failure;
    }
    std::lock_guard<std::mutex> lck(listMutex_);
    for (auto &syncObserver : syncObservers_) {
        syncObserver->Clear();
    }
    syncObservers_.clear();
    return napi_ok;
}

napi_status JsKVStore::Subscribe(uint8_t type, std::shared_ptr<DataObserver> observer)
{
    auto subscribeType = ToSubscribeType(type);
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    ZLOGD("kvStore_->SubscribeKvStore(%{public}d) return %{public}d", type, status);
    if (status != Status::SUCCESS) {
        observer->Clear();
        return napi_generic_failure;
    }
    dataObserver_[type].push_back(observer);
    return napi_ok;
}

napi_status JsKVStore::UnSubscribe(uint8_t type, std::shared_ptr<DataObserver> observer)
{
    auto subscribeType = ToSubscribeType(type);
    Status status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    ZLOGD("kvStore_->UnSubscribeKvStore(%{public}d) return %{public}d", type, status);
    if (status == Status::SUCCESS) {
        observer->Clear();
        return napi_ok;
    }
    return napi_generic_failure;
}

void JsKVStore::SetUvQueue(std::shared_ptr<UvQueue> uvQueue)
{
    uvQueue_ = uvQueue;
}

void JsKVStore::DataObserver::OnChange(const ChangeNotification &notification,
                                       std::shared_ptr<KvStoreSnapshot> snapshot)
{
    ZLOGD("data change insert:%{public}zu, update:%{public}zu, delete:%{public}zu",
        notification.GetInsertEntries().size(), notification.GetUpdateEntries().size(),
        notification.GetDeleteEntries().size());
}

void JsKVStore::DataObserver::OnChange(const ChangeNotification& notification)
{
    ZLOGD("data change insert:%{public}zu, update:%{public}zu, delete:%{public}zu",
        notification.GetInsertEntries().size(), notification.GetUpdateEntries().size(),
        notification.GetDeleteEntries().size());
    KvStoreObserver::OnChange(notification);

    auto args = [notification](napi_env env, int& argc, napi_value* argv) {
        // generate 1 arguments for callback function.
        argc = 1;
        JSUtil::SetValue(env, notification, argv[0]);
    };
    AsyncCall(args);
}

void JsKVStore::SyncObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    auto args = [results](napi_env env, int& argc, napi_value* argv) {
        // generate 1 arguments for callback function.
        argc = 1;
        JSUtil::SetValue(env, results, argv[0]);
    };
    AsyncCall(args);
}
} // namespace OHOS::DistributedData

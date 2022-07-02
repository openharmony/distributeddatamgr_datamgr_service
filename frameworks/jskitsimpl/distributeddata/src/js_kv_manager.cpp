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
#define LOG_TAG "JS_KVManager"
#include "js_kv_manager.h"
#include "distributed_kv_data_manager.h"
#include "js_device_kv_store.h"
#include "js_single_kv_store.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedData {
bool IsStoreTypeSupported(Options options)
{
    return (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION)
        || (options.kvStoreType == KvStoreType::SINGLE_VERSION);
}

JsKVManager::JsKVManager(const std::string &bundleName, napi_env env, ContextParam param)
    : bundleName_(bundleName), uvQueue_(std::make_shared<UvQueue>(env)), param_(std::move(param))
{
}

JsKVManager::~JsKVManager()
{
    ZLOGD("no memory leak for JsKVManager");
    std::lock_guard<std::mutex> lck(deathMutex_);
    for (auto& it : deathRecipient_) {
        kvDataManager_.UnRegisterKvStoreServiceDeathRecipient(it);
        it->Clear();
    }
    deathRecipient_.clear();
}

/*
 * [JS API Prototype]
 * [AsyncCB]  createKVManager(config: KVManagerConfig, callback: AsyncCallback<JsKVManager>): void;
 * [Promise]  createKVManager(config: KVManagerConfig) : Promise<JsKVManager>;
 */
napi_value JsKVManager::CreateKVManager(napi_env env, napi_callback_info info)
{
    ZLOGD("CreateKVManager in");
    struct ContextInfo : public ContextBase {
        JsKVManager* kvManger = nullptr;
        napi_ref ref = nullptr;
    };
    auto ctxt = std::make_shared<ContextInfo>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <bundleName>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        std::string bundleName;
        ctxt->status = JSUtil::GetNamedProperty(env, argv[0], "bundleName", bundleName);
        CHECK_ARGS_RETURN_VOID(ctxt, (ctxt->status == napi_ok) && !bundleName.empty(), "invalid bundleName!");

        ctxt->ref = JSUtil::NewWithRef(env, argc, argv, reinterpret_cast<void**>(&ctxt->kvManger),
                                       JsKVManager::Constructor(env));
        CHECK_ARGS_RETURN_VOID(ctxt, ctxt->kvManger != nullptr, "KVManager::New failed!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto noExecute = NapiAsyncExecute();
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        CHECK_STATUS_RETURN_VOID(ctxt, "output KVManager failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), noExecute, output);
}

struct GetKVStoreContext : public ContextBase {
    std::string storeId;
    Options options;
    JsKVStore* kvStore = nullptr;
    napi_ref ref = nullptr;

    void GetCbInfo(napi_env env, napi_callback_info info)
    {
        auto input = [env, this](size_t argc, napi_value* argv) {
            // required 2 arguments :: <storeId> <options>
            CHECK_ARGS_RETURN_VOID(this, argc == 2, "invalid arguments!");
            status = JSUtil::GetValue(env, argv[0], storeId);
            CHECK_ARGS_RETURN_VOID(this, (status == napi_ok) && !storeId.empty(), "invalid storeId!");
            status = JSUtil::GetValue(env, argv[1], options);
            CHECK_STATUS_RETURN_VOID(this, "invalid options!");
            CHECK_ARGS_RETURN_VOID(this, IsStoreTypeSupported(options), "invalid options.KvStoreType");
            ZLOGD("GetKVStore kvStoreType=%{public}d", options.kvStoreType);
            if (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION) {
                ref = JSUtil::NewWithRef(env, argc, argv, reinterpret_cast<void**>(&kvStore),
                                         JsDeviceKVStore::Constructor(env));
            } else if (options.kvStoreType == KvStoreType::SINGLE_VERSION) {
                ref = JSUtil::NewWithRef(env, argc, argv, reinterpret_cast<void**>(&kvStore),
                                         JsSingleKVStore::Constructor(env));
            }
        };
        ContextBase::GetCbInfo(env, info, input);
    }
};

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      getKVStore<T extends KVStore>(storeId: string, options: Options, callback: AsyncCallback<T>): void;
 * [Promise]
 *      getKVStore<T extends KVStore>(storeId: string, options: Options): Promise<T>;
 */
napi_value JsKVManager::GetKVStore(napi_env env, napi_callback_info info)
{
    ZLOGD("GetKVStore in");
    auto ctxt = std::make_shared<GetKVStoreContext>();
    ctxt->GetCbInfo(env, info);
    auto execute = [ctxt]() {
        auto kvm = reinterpret_cast<JsKVManager*>(ctxt->native);
        CHECK_ARGS_RETURN_VOID(ctxt, kvm != nullptr, "KVManager is null, failed!");
        AppId appId = { kvm->bundleName_ };
        StoreId storeId = { ctxt->storeId };
        ctxt->options.baseDir = kvm->param_.baseDir;
        ctxt->options.area = kvm->param_.area + 1;
        ctxt->options.hapName = kvm->param_.hapName;
        ZLOGD("Options area:%{public}d dir:%{public}s",ctxt->options.area, ctxt->options.baseDir.c_str());
        std::shared_ptr<DistributedKv::SingleKvStore> kvStore;
        Status status = kvm->kvDataManager_.GetSingleKvStore(ctxt->options, appId, storeId, kvStore);
        ZLOGD("GetSingleKvStore status:%{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "GetSingleKvStore() failed!");
        ctxt->kvStore->SetNative(kvStore);
        ctxt->kvStore->SetUvQueue(kvm->uvQueue_);
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        CHECK_STATUS_RETURN_VOID(ctxt, "output KvStore failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 * [AsyncCB]  closeKVStore(appId: string, storeId: string, kvStore: KVStore, callback: AsyncCallback<void>):void
 * [Promise]  closeKVStore(appId: string, storeId: string, kvStore: KVStore):Promise<void>
 */
napi_value JsKVManager::CloseKVStore(napi_env env, napi_callback_info info)
{
    ZLOGD("CloseKVStore in");
    struct ContextInfo : public ContextBase {
        std::string appId;
        std::string storeId;
        napi_value kvStore;
    };
    auto ctxt = std::make_shared<ContextInfo>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 3 arguments :: <appId> <storeId> <kvStore>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 3, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->appId);
        CHECK_ARGS_RETURN_VOID(ctxt, (ctxt->status == napi_ok) && !ctxt->appId.empty(), "invalid appId!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->storeId);
        CHECK_ARGS_RETURN_VOID(ctxt, (ctxt->status == napi_ok) && !ctxt->storeId.empty(), "invalid storeId!");
        CHECK_ARGS_RETURN_VOID(ctxt, argv[2] != nullptr, "kvStore is nullptr!");
        bool isSingle = JsKVStore::IsInstanceOf(env, argv[2], ctxt->storeId, JsSingleKVStore::Constructor(env));
        bool isDevice = JsKVStore::IsInstanceOf(env, argv[2], ctxt->storeId, JsDeviceKVStore::Constructor(env));
        CHECK_ARGS_RETURN_VOID(ctxt, isSingle || isDevice, "kvStore unmatch to storeId!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        AppId appId { ctxt->appId };
        StoreId storeId { ctxt->storeId };
        Status status = reinterpret_cast<JsKVManager*>(ctxt->native)->kvDataManager_.CloseKvStore(appId, storeId);
        ZLOGD("CloseKVStore return status:%{public}d", status);
        ctxt->status
            = ((status == Status::SUCCESS) || (status == Status::STORE_NOT_FOUND) || (status == Status::STORE_NOT_OPEN))
            ? napi_ok
            : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCB]  deleteKVStore(appId: string, storeId: string, callback: AsyncCallback<void>): void
 * [Promise]  deleteKVStore(appId: string, storeId: string):Promise<void>
 */
napi_value JsKVManager::DeleteKVStore(napi_env env, napi_callback_info info)
{
    ZLOGD("DeleteKVStore in");
    struct ContextInfo : public ContextBase {
        std::string appId;
        std::string storeId;
    };
    auto ctxt = std::make_shared<ContextInfo>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <appId> <storeId>
        CHECK_ARGS_RETURN_VOID(ctxt, argc >= 2, "invalid arguments!");
        size_t index = 0;
        ctxt->status = JSUtil::GetValue(env, argv[index++], ctxt->appId);
        CHECK_ARGS_RETURN_VOID(ctxt, !ctxt->appId.empty(), "invalid appId");
        ctxt->status = JSUtil::GetValue(env, argv[index++], ctxt->storeId);
        CHECK_ARGS_RETURN_VOID(ctxt, !ctxt->storeId.empty(), "invalid storeId");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        AppId appId { ctxt->appId };
        StoreId storeId { ctxt->storeId };
        auto kvm = reinterpret_cast<JsKVManager*>(ctxt->native);
        CHECK_ARGS_RETURN_VOID(ctxt, kvm != nullptr, "KVManager is null, failed!");
        std::string databaseDir = kvm->param_.baseDir;
        ZLOGD("DeleteKVStore databaseDir is: %{public}s", databaseDir.c_str());
        Status status = kvm->kvDataManager_.DeleteKvStore(appId, storeId, databaseDir);
        ZLOGD("DeleteKvStore status:%{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCB]  getAllKVStoreId(appId: string, callback: AsyncCallback<string[]>):void
 * [Promise]  getAllKVStoreId(appId: string):Promise<string[]>
 */
napi_value JsKVManager::GetAllKVStoreId(napi_env env, napi_callback_info info)
{
    ZLOGD("GetAllKVStoreId in");
    struct ContextInfo : public ContextBase {
        std::string appId;
        std::vector<StoreId> storeIdList;
    };

    auto ctxt = std::make_shared<ContextInfo>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <appId>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->appId);
        CHECK_ARGS_RETURN_VOID(ctxt, !ctxt->appId.empty(), "invalid appId!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto kvm = reinterpret_cast<JsKVManager*>(ctxt->native);
        CHECK_ARGS_RETURN_VOID(ctxt, kvm != nullptr, "KVManager is null, failed!");
        AppId appId { ctxt->appId };
        Status status = kvm->kvDataManager_.GetAllKvStoreId(appId, ctxt->storeIdList);
        ZLOGD("execute status:%{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->storeIdList, result);
        ZLOGD("output status:%{public}d", ctxt->status);
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

napi_value JsKVManager::On(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <event> <callback>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        ZLOGI("subscribe to event:%{public}s", event.c_str());
        CHECK_ARGS_RETURN_VOID(ctxt, event == "distributedDataServiceDie", "invalid arg[0], i.e. invalid event!");

        napi_valuetype valueType = napi_undefined;
        ctxt->status = napi_typeof(env, argv[1], &valueType);
        CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
        CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "callback is not a function");

        JsKVManager* proxy = reinterpret_cast<JsKVManager*>(ctxt->native);
        CHECK_ARGS_RETURN_VOID(ctxt, proxy != nullptr, "there is no native kv manager");

        std::lock_guard<std::mutex> lck(proxy->deathMutex_);
        for (auto& it : proxy->deathRecipient_) {
            if (JSUtil::Equals(env, argv[1], it->GetCallback())) {
                ZLOGD("KVManager::On callback already register!");
                return;
            }
        }
        auto deathRecipient = std::make_shared<DeathRecipient>(proxy->uvQueue_, argv[1]);
        proxy->kvDataManager_.RegisterKvStoreServiceDeathRecipient(deathRecipient);
        proxy->deathRecipient_.push_back(deathRecipient);
        ZLOGD("on mapsize: %{public}d", static_cast<int>(proxy->deathRecipient_.size()));
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return nullptr;
}

napi_value JsKVManager::Off(napi_env env, napi_callback_info info)
{
    ZLOGD("KVManager::Off()");
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 or 2 arguments :: <event> [callback]
        CHECK_ARGS_RETURN_VOID(ctxt, (argc == 1) || (argc == 2), "invalid arguments!");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        // required 1 arguments :: <event>
        ZLOGI("unsubscribe to event:%{public}s %{public}s specified", event.c_str(), (argc == 1) ? "without": "with");
        CHECK_ARGS_RETURN_VOID(ctxt, event == "distributedDataServiceDie", "invalid arg[0], i.e. invalid event!");
        // have 2 arguments :: have the [callback]
        if (argc == 2) {
            napi_valuetype valueType = napi_undefined;
            ctxt->status = napi_typeof(env, argv[1], &valueType);
            CHECK_STATUS_RETURN_VOID(ctxt, "napi_typeof failed!");
            CHECK_ARGS_RETURN_VOID(ctxt, valueType == napi_function, "callback is not a function");
        }
        JsKVManager* proxy = reinterpret_cast<JsKVManager*>(ctxt->native);
        std::lock_guard<std::mutex> lck(proxy->deathMutex_);
        auto it = proxy->deathRecipient_.begin();
        while (it != proxy->deathRecipient_.end()) {
            // have 2 arguments :: have the [callback]
            if ((argc == 1) || JSUtil::Equals(env, argv[1], (*it)->GetCallback())) {
                proxy->kvDataManager_.UnRegisterKvStoreServiceDeathRecipient(*it);
                (*it)->Clear();
                it = proxy->deathRecipient_.erase(it);
            } else {
                ++it;
            }
        }
        ZLOGD("off mapsize: %{public}d", static_cast<int>(proxy->deathRecipient_.size()));
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    ZLOGD("KVManager::Off callback is not register or already unregister!");
    return nullptr;
}

napi_value JsKVManager::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getKVStore", JsKVManager::GetKVStore),
        DECLARE_NAPI_FUNCTION("closeKVStore", JsKVManager::CloseKVStore),
        DECLARE_NAPI_FUNCTION("deleteKVStore", JsKVManager::DeleteKVStore),
        DECLARE_NAPI_FUNCTION("getAllKVStoreId", JsKVManager::GetAllKVStoreId),
        DECLARE_NAPI_FUNCTION("on", JsKVManager::On),
        DECLARE_NAPI_FUNCTION("off", JsKVManager::Off)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "KVManager", properties, count, JsKVManager::New);
}

napi_value JsKVManager::New(napi_env env, napi_callback_info info)
{
    std::string bundleName;
    ContextParam param;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &bundleName, &param](size_t argc, napi_value* argv) {
        // required 1 arguments :: <bundleName>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetNamedProperty(env, argv[0], "bundleName", bundleName);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid bundleName!");
        CHECK_ARGS_RETURN_VOID(ctxt, !bundleName.empty(), "invalid arg[0], i.e. invalid bundleName!");

        napi_value jsContext = nullptr;
        JSUtil::GetNamedProperty(env, argv[0], "context", jsContext);
        ctxt->status = JSUtil::GetValue(env, jsContext, param);
        CHECK_ARGS_RETURN_VOID(ctxt, ctxt->status == napi_ok, "get context failed");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    JsKVManager* kvManager = new (std::nothrow) JsKVManager(bundleName, env, param);
    NAPI_ASSERT(env, kvManager !=nullptr, "no memory for kvManager");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("kvManager finalize.");
        auto* kvManager = reinterpret_cast<JsKVManager*>(data);
        CHECK_RETURN_VOID(kvManager != nullptr, "finalize null!");
        delete kvManager;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, kvManager, finalize, nullptr, nullptr));
    return ctxt->self;
}

void JsKVManager::DeathRecipient::OnRemoteDied()
{
    AsyncCall();
}
} // namespace OHOS::DistributedData

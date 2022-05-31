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
#define LOG_TAG "JS_DeviceKVStore"
#include "js_device_kv_store.h"
#include <iomanip>
#include "kv_utils.h"
#include "js_kv_store_resultset.h"
#include "datashare_predicates.h"
#include "js_query.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"
#include "distributed_kv_data_manager.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::DistributedData {
constexpr int DEVICEID_WIDTH = 4;
static std::string GetDeviceKey(const std::string& deviceId, const std::string& key)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(DEVICEID_WIDTH) << deviceId.length();
    oss << deviceId << key;
    return oss.str();
}

JsDeviceKVStore::JsDeviceKVStore(const std::string& storeId)
    : JsKVStore(storeId)
{
}

napi_value JsDeviceKVStore::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("put", JsKVStore::Put),
        DECLARE_NAPI_FUNCTION("delete", JsKVStore::Delete),
        DECLARE_NAPI_FUNCTION("putBatch", JsKVStore::PutBatch),
        DECLARE_NAPI_FUNCTION("deleteBatch", JsKVStore::DeleteBatch),
        DECLARE_NAPI_FUNCTION("startTransaction", JsKVStore::StartTransaction),
        DECLARE_NAPI_FUNCTION("commit", JsKVStore::Commit),
        DECLARE_NAPI_FUNCTION("rollback", JsKVStore::Rollback),
        DECLARE_NAPI_FUNCTION("enableSync", JsKVStore::EnableSync),
        DECLARE_NAPI_FUNCTION("setSyncRange", JsKVStore::SetSyncRange),
        /* JsDeviceKVStore externs JsKVStore */
        DECLARE_NAPI_FUNCTION("get", JsDeviceKVStore::Get),
        DECLARE_NAPI_FUNCTION("getEntries", JsDeviceKVStore::GetEntries),
        DECLARE_NAPI_FUNCTION("getResultSet", JsDeviceKVStore::GetResultSet),
        DECLARE_NAPI_FUNCTION("closeResultSet", JsDeviceKVStore::CloseResultSet),
        DECLARE_NAPI_FUNCTION("getResultSize", JsDeviceKVStore::GetResultSize),
        DECLARE_NAPI_FUNCTION("removeDeviceData", JsDeviceKVStore::RemoveDeviceData),
        DECLARE_NAPI_FUNCTION("sync", JsDeviceKVStore::Sync),
        DECLARE_NAPI_FUNCTION("on", JsKVStore::OnEvent), /* same to JsSingleKVStore */
        DECLARE_NAPI_FUNCTION("off", JsKVStore::OffEvent) /* same to JsSingleKVStore */
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);

    return JSUtil::DefineClass(env, "DeviceKVStore", properties, count, JsDeviceKVStore::New);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      get(deviceId:string, key:string, callback:AsyncCallback<boolean|string|number|Uint8Array>):void;
 * [Promise]
 *      get(deviceId:string, key:string):Promise<boolean|string|number|Uint8Array>;
 */
napi_value JsDeviceKVStore::Get(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::get()");
    struct GetContext : public ContextBase {
        std::string deviceId;
        std::string key;
        JSUtil::KvStoreVariant value;
    };
    auto ctxt = std::make_shared<GetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // number 2 means: required 2 arguments, <deviceId> + <key>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid deviceId!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->key);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid key!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        std::string deviceKey = GetDeviceKey(ctxt->deviceId, ctxt->key);
        OHOS::DistributedKv::Key key(deviceKey);
        OHOS::DistributedKv::Value value;
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        if (kvStore == nullptr) {
            ZLOGE("kvStore is nullptr");
            return;
        }
        Status status = kvStore->Get(key, value);
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->result() failed!");
        ZLOGD("kvStore->Get return %{public}d", status);
        ctxt->value = JSUtil::Blob2VariantValue(value);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->Get() failed!");
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->value, result);
        CHECK_STATUS_RETURN_VOID(ctxt, "output failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

enum class ArgsType : uint8_t {
    /* input arguments' combination type */
    DEVICEID_KEYPREFIX = 0,
    DEVICEID_QUERY,
    QUERY,
    UNKNOWN = 255
};
struct VariantArgs {
    /* input arguments' combinations */
    std::string deviceId;
    std::string keyPrefix;
    JsQuery* query;
    ArgsType type = ArgsType::UNKNOWN;
    DataQuery dataQuery;
};

static napi_status GetVariantArgs(napi_env env, size_t argc, napi_value* argv, VariantArgs& va)
{
    CHECK_RETURN(argc > 0, "invalid arguments!", napi_invalid_arg);
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, argv[0], &type);
    CHECK_RETURN((type == napi_string) || (type == napi_object), "invalid arg[0], type error!", napi_invalid_arg);
    if (type == napi_string) {
        // number 2 means: required 2 arguments, <deviceId> <keyPrefix/query>
        CHECK_RETURN(argc == 2, "invalid arguments!", napi_invalid_arg);
        status = JSUtil::GetValue(env, argv[0], va.deviceId);
        CHECK_RETURN(!va.deviceId.empty(), "invalid arg[0], i.e. invalid deviceId!", napi_invalid_arg);
        status = napi_typeof(env, argv[1], &type);
        CHECK_RETURN((type == napi_string) || (type == napi_object), "invalid arg[1], type error!", napi_invalid_arg);
        if (type == napi_string) {
            status = JSUtil::GetValue(env, argv[1], va.keyPrefix);
            CHECK_RETURN(!va.keyPrefix.empty(), "invalid arg[1], i.e. invalid keyPrefix!", napi_invalid_arg);
            va.type = ArgsType::DEVICEID_KEYPREFIX;
        } else if (type == napi_object) {
            bool result = false;
            status = napi_instanceof(env, argv[1], JsQuery::Constructor(env), &result);
            if ((status == napi_ok) && (result != false)) {
                status = JSUtil::Unwrap(env, argv[1], reinterpret_cast<void**>(&va.query), JsQuery::Constructor(env));
                CHECK_RETURN(va.query != nullptr, "invalid arg[1], i.e. invalid query!", napi_invalid_arg);
                va.type = ArgsType::DEVICEID_QUERY;
            } else {
                status = JSUtil::GetValue(env, argv[1], va.dataQuery);
                ZLOGD("kvStoreDataShare->GetResultSet return %{public}d", status);
                CHECK_RETURN(true, "invalid arg[0], i.e. invalid predicates!", napi_invalid_arg);
            }
        }
    } else if (type == napi_object) {
        // number 1 means: required 1 arguments, <query>
        CHECK_RETURN(argc == 1, "invalid arguments!", napi_invalid_arg);
        bool result = false;
        status = napi_instanceof(env, argv[0], JsQuery::Constructor(env), &result);
        if ((status == napi_ok) && (result != false)) {
            status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&va.query), JsQuery::Constructor(env));
            CHECK_RETURN(va.query != nullptr, "invalid arg[0], i.e. invalid query!", napi_invalid_arg);
            va.type = ArgsType::QUERY;
        } else {
            DeviceInfo info;
            DistributedKvDataManager manager;
            Status daviceStatus = manager.GetLocalDevice(info);
            if (daviceStatus != Status::SUCCESS) {
                ZLOGD("GetLocalDevice return %{public}d", daviceStatus);
            }
            va.deviceId = info.deviceId;
            status = JSUtil::GetValue(env, argv[0], va.dataQuery);
            ZLOGD("GetResultSet arguement[0] return %{public}d", status);
        }
    }
    return status;
};

/*
 * [JS API Prototype]
 *  getEntries(deviceId:string, keyPrefix:string, callback:AsyncCallback<Entry[]>):void
 *  getEntries(deviceId:string, keyPrefix:string):Promise<Entry[]>
 *
 *  getEntries(query:Query, callback:AsyncCallback<Entry[]>):void
 *  getEntries(query:Query) : Promise<Entry[]>
 *
 *  getEntries(deviceId:string, query:Query):callback:AsyncCallback<Entry[]>):void
 *  getEntries(deviceId:string, query:Query):Promise<Entry[]>
 */
napi_value JsDeviceKVStore::GetEntries(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetEntries()");
    struct GetEntriesContext : public ContextBase {
        VariantArgs va;
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<GetEntriesContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arguments!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        Status status = Status::INVALID_ARGUMENT;
        if (ctxt->va.type == ArgsType::DEVICEID_KEYPREFIX) {
            std::string deviceKey = GetDeviceKey(ctxt->va.deviceId, ctxt->va.keyPrefix);
            OHOS::DistributedKv::Key keyPrefix(deviceKey);
            status = kvStore->GetEntries(keyPrefix, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            auto query = ctxt->va.query->GetNative();
            query.DeviceId(ctxt->va.deviceId);
            status = kvStore->GetEntries(query, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::QUERY) {
            auto query = ctxt->va.query->GetNative();
            status = kvStore->GetEntries(query, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        }
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->GetEntries() failed!");
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->entries, result);
        CHECK_STATUS_RETURN_VOID(ctxt, "output failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  getResultSet(deviceId:string, keyPrefix:string, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(deviceId:string, keyPrefix:string):Promise<KvStoreResultSet>
 *
 *  getResultSet(query:Query, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(query:Query):Promise<KvStoreResultSet>
 *
 *  getResultSet(deviceId:string, query:Query, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(deviceId:string, query:Query):Promise<KvStoreResultSet>
 */
napi_value JsDeviceKVStore::GetResultSet(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetResultSet()");
    struct GetResultSetContext : public ContextBase {
        VariantArgs va;
        JsKVStoreResultSet* resultSet = nullptr;
        napi_ref ref = nullptr;
    };
    auto ctxt = std::make_shared<GetResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arguments!");
        ctxt->ref = JSUtil::NewWithRef(env, 0, nullptr, reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        CHECK_ARGS_RETURN_VOID(ctxt, ctxt->resultSet != nullptr, "KVStoreResultSet::New failed!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        std::shared_ptr<KvStoreResultSet> kvResultSet;
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        Status status = Status::INVALID_ARGUMENT;
        if (ctxt->va.type == ArgsType::DEVICEID_KEYPREFIX) {
            std::string deviceKey = GetDeviceKey(ctxt->va.deviceId, ctxt->va.keyPrefix);
            OHOS::DistributedKv::Key keyPrefix(deviceKey);
            status = kvStore->GetResultSet(keyPrefix, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            auto query = ctxt->va.query->GetNative();
            query.DeviceId(ctxt->va.deviceId);
            status = kvStore->GetResultSet(query, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::QUERY) {
            auto query = ctxt->va.query->GetNative();
            status = kvStore->GetResultSet(query, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else {
            ctxt->va.dataQuery.DeviceId(ctxt->va.deviceId);
            ZLOGD("ArgsType::DEVICEID_PREDICATES ToQuery return %{public}d", status);
            status = kvStore->GetResultSet(ctxt->va.dataQuery, kvResultSet);
            ZLOGD("ArgsType::DEVICEID_PREDICATES GetResultSetWithQuery return %{public}d", status);
        };
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->GetResultSet() failed!");
        ctxt->resultSet->SetNative(kvResultSet);
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        CHECK_STATUS_RETURN_VOID(ctxt, "output KvResultSet failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  closeResultSet(resultSet:KVStoreResultSet, callback: AsyncCallback<void>):void
 *  closeResultSet(resultSet:KVStoreResultSet):Promise<void>
 */
napi_value JsDeviceKVStore::CloseResultSet(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::CloseResultSet()");
    struct CloseResultSetContext : public ContextBase {
        JsKVStoreResultSet* resultSet = nullptr;
    };
    auto ctxt = std::make_shared<CloseResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <resultSet>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        napi_valuetype type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &type);
        CHECK_ARGS_RETURN_VOID(ctxt, type == napi_object, "invalid arg[0], i.e. invalid resultSet!");
        ctxt->status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        CHECK_ARGS_RETURN_VOID(ctxt, ctxt->resultSet != nullptr, "invalid arg[0], i.e. invalid resultSet!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        Status status = kvStore->CloseResultSet(ctxt->resultSet->GetNative());
        ZLOGD("kvStore->CloseResultSet return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->CloseResultSet failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 *  getResultSize(query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(query:Query):Promise<number>
 *
 *  getResultSize(deviceId:string, query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(deviceId:string, query:Query):Promise<number>
 */
napi_value JsDeviceKVStore::GetResultSize(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetResultSize()");
    struct ResultSizeContext : public ContextBase {
        VariantArgs va;
        int resultSize = 0;
    };
    auto ctxt = std::make_shared<ResultSizeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_ARGS_RETURN_VOID(ctxt, (ctxt->va.type == ArgsType::DEVICEID_QUERY) || (ctxt->va.type == ArgsType::QUERY),
            "invalid arguments!");
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arguments!");
    };

    ctxt->GetCbInfo(env, info, input);
    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        auto query = ctxt->va.query->GetNative();
        if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            query.DeviceId(ctxt->va.deviceId);
        }
        Status status = kvStore->GetCount(query, ctxt->resultSize);
        ZLOGD("kvStore->GetCount() return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->GetCount() failed!");
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, static_cast<int32_t>(ctxt->resultSize), result);
        CHECK_STATUS_RETURN_VOID(ctxt, "output resultSize failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  removeDeviceData(deviceId:string, callback: AsyncCallback<void>):void
 *  removeDeviceData(deviceId:string):Promise<void>
 */
napi_value JsDeviceKVStore::RemoveDeviceData(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::RemoveDeviceData()");
    struct RemoveDeviceContext : public ContextBase {
        std::string deviceId;
    };
    auto ctxt = std::make_shared<RemoveDeviceContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <deviceId>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid deviceId!");
    };
    ctxt->GetCbInfo(env, info, input);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
        Status status = kvStore->RemoveDeviceData(ctxt->deviceId);
        ZLOGD("kvStore->RemoveDeviceData return %{public}d", status);
        ctxt->status = (status == Status::SUCCESS) ? napi_ok : napi_generic_failure;
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->RemoveDeviceData() failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 *  sync(deviceIdList:string[], mode:SyncMode, allowedDelayMs?:number):void
 */
napi_value JsDeviceKVStore::Sync(napi_env env, napi_callback_info info)
{
    struct SyncContext : public ContextBase {
        std::vector<std::string> deviceIdList;
        uint32_t mode = 0;
        uint32_t allowedDelayMs = 0;
        JsQuery* query = nullptr;
        napi_valuetype type = napi_undefined;
    };
    auto ctxt = std::make_shared<SyncContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 3 arguments :: <deviceIdList> <mode> [allowedDelayMs]
        CHECK_ARGS_RETURN_VOID(ctxt, (argc == 2) || (argc == 3) || (argc == 4), "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceIdList);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid deviceIdList!");
        napi_typeof(env, argv[1], &ctxt->type);   
        if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::Unwrap(env, argv[1], reinterpret_cast<void**>(&ctxt->query), JsQuery::Constructor(env));
            CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid mode!");
            ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->mode);
        }
        if (ctxt->type == napi_number) {
            ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->mode);
            if (argc == 3) {
                ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->allowedDelayMs);
            }
        }
        CHECK_ARGS_RETURN_VOID(ctxt, ctxt->mode <= uint32_t(SyncMode::PUSH_PULL), "invalid arg[1], i.e. invalid mode!");
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[2], i.e. invalid arguement[2]!");       
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetNative();
    Status status = Status::INVALID_ARGUMENT;
    if (ctxt->type == napi_object) {
        auto query = ctxt->query->GetNative();
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), query, nullptr);
    }
    if (ctxt->type == napi_number) {
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), ctxt->allowedDelayMs);
    }
    ZLOGD("kvStore->Sync return %{public}d!", status);
    NAPI_ASSERT(env, status == Status::SUCCESS, "kvStore->Sync() failed!");
    return nullptr;
}

napi_value JsDeviceKVStore::New(napi_env env, napi_callback_info info)
{
    ZLOGD("Constructor single kv store!");
    std::string storeId;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &storeId](size_t argc, napi_value* argv) {
        // required 2 arguments :: <storeId> <options>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], storeId);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid storeId!");
        CHECK_ARGS_RETURN_VOID(ctxt, !storeId.empty(), "invalid arg[0], i.e. invalid storeId!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    JsDeviceKVStore* kvStore = new (std::nothrow) JsDeviceKVStore(storeId);
    NAPI_ASSERT(env, kvStore !=nullptr, "no memory for kvStore");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("deviceKvStore finalize.");
        auto* kvStore = reinterpret_cast<JsDeviceKVStore*>(data);
        CHECK_RETURN_VOID(kvStore != nullptr, "finalize null!");
        delete kvStore;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, kvStore, finalize, nullptr, nullptr));
    return ctxt->self;
}
} // namespace OHOS::DistributedData

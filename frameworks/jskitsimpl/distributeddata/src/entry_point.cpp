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
#define LOG_TAG "EntryPoint"
#include "js_field_node.h"
#include "js_kv_manager.h"
#include "js_kv_store.h"
#include "js_query.h"
#include "js_schema.h"
#include "js_util.h"
#include "log_print.h"

using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;

static napi_status SetNamedProperty(napi_env env, napi_value& obj, const std::string& name, int32_t value)
{
    napi_value property = nullptr;
    napi_status status = napi_create_int32(env, value, &property);
    ZLOGE_RETURN(status == napi_ok, "int32_t to napi_value failed!", status);
    status = napi_set_named_property(env, obj, name.c_str(), property);
    ZLOGE_RETURN(status == napi_ok, "napi_set_named_property failed!", status);
    return status;
}

static napi_value ExportUserType(napi_env env)
{
    constexpr int32_t SAME_USER_ID = 0;

    napi_value userType = nullptr;
    napi_create_object(env, &userType);
    SetNamedProperty(env, userType, "SAME_USER_ID", SAME_USER_ID);
    return userType;
}

static napi_value ExportConstants(napi_env env)
{
    constexpr int32_t MAX_KEY_LENGTH = 1024;
    constexpr int32_t MAX_VALUE_LENGTH = 4194303;
    constexpr int32_t MAX_KEY_LENGTH_DEVICE = 896;
    constexpr int32_t MAX_STORE_ID_LENGTH = 128;
    constexpr int32_t MAX_QUERY_LENGTH = 512000;
    constexpr int32_t MAX_BATCH_SIZE = 128;

    napi_value constants = nullptr;
    napi_create_object(env, &constants);
    SetNamedProperty(env, constants, "MAX_KEY_LENGTH", MAX_KEY_LENGTH);
    SetNamedProperty(env, constants, "MAX_VALUE_LENGTH", MAX_VALUE_LENGTH);
    SetNamedProperty(env, constants, "MAX_KEY_LENGTH_DEVICE", MAX_KEY_LENGTH_DEVICE);
    SetNamedProperty(env, constants, "MAX_STORE_ID_LENGTH", MAX_STORE_ID_LENGTH);
    SetNamedProperty(env, constants, "MAX_QUERY_LENGTH", MAX_QUERY_LENGTH);
    SetNamedProperty(env, constants, "MAX_BATCH_SIZE", MAX_BATCH_SIZE);
    return constants;
}

static napi_value ExportValueType(napi_env env)
{
    napi_value valueType = nullptr;
    napi_create_object(env, &valueType);
    SetNamedProperty(env, valueType, "STRING", (int32_t)JSUtil::STRING);
    SetNamedProperty(env, valueType, "INTEGER", (int32_t)JSUtil::INTEGER);
    SetNamedProperty(env, valueType, "FLOAT", (int32_t)JSUtil::FLOAT);
    SetNamedProperty(env, valueType, "BYTE_ARRAY", (int32_t)JSUtil::BYTE_ARRAY);
    SetNamedProperty(env, valueType, "BOOLEAN", (int32_t)JSUtil::BOOLEAN);
    SetNamedProperty(env, valueType, "DOUBLE", (int32_t)JSUtil::DOUBLE);
    return valueType;
}

static napi_value ExportSyncMode(napi_env env)
{
    napi_value syncMode = nullptr;
    napi_create_object(env, &syncMode);
    SetNamedProperty(env, syncMode, "PULL_ONLY", (int32_t)SyncMode::PULL);
    SetNamedProperty(env, syncMode, "PUSH_ONLY", (int32_t)SyncMode::PUSH);
    SetNamedProperty(env, syncMode, "PUSH_PULL", (int32_t)SyncMode::PUSH_PULL);
    return syncMode;
}

static napi_value ExportSubscribeType(napi_env env)
{
    napi_value subscribeType = nullptr;
    napi_create_object(env, &subscribeType);

    SetNamedProperty(env, subscribeType, "SUBSCRIBE_TYPE_LOCAL", (int32_t)SUBSCRIBE_LOCAL);
    SetNamedProperty(env, subscribeType, "SUBSCRIBE_TYPE_REMOTE", (int32_t)SUBSCRIBE_REMOTE);
    SetNamedProperty(env, subscribeType, "SUBSCRIBE_TYPE_ALL", (int32_t)SUBSCRIBE_LOCAL_REMOTE);
    return subscribeType;
}

static napi_value ExportKVStoreType(napi_env env)
{
    napi_value kvStoreType = nullptr;
    napi_create_object(env, &kvStoreType);
    SetNamedProperty(env, kvStoreType, "DEVICE_COLLABORATION", (int32_t)KvStoreType::DEVICE_COLLABORATION);
    SetNamedProperty(env, kvStoreType, "SINGLE_VERSION", (int32_t)KvStoreType::SINGLE_VERSION);
    SetNamedProperty(env, kvStoreType, "MULTI_VERSION", (int32_t)KvStoreType::MULTI_VERSION);
    return kvStoreType;
}

static napi_value ExportSecurityLevel(napi_env env)
{
    napi_value securityLevel = nullptr;
    napi_create_object(env, &securityLevel);
    SetNamedProperty(env, securityLevel, "NO_LEVEL", (int32_t)SecurityLevel::NO_LABEL);
    SetNamedProperty(env, securityLevel, "S0", (int32_t)SecurityLevel::S0);
    SetNamedProperty(env, securityLevel, "S1", (int32_t)SecurityLevel::S1);
    SetNamedProperty(env, securityLevel, "S2", (int32_t)SecurityLevel::S2);
    SetNamedProperty(env, securityLevel, "S3", (int32_t)SecurityLevel::S3);
    SetNamedProperty(env, securityLevel, "S4", (int32_t)SecurityLevel::S4);
    return securityLevel;
}

static napi_status InitEnumerates(napi_env env, napi_value exports)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_PROPERTY("UserType", ExportUserType(env)),
        DECLARE_NAPI_PROPERTY("Constants", ExportConstants(env)),
        DECLARE_NAPI_PROPERTY("ValueType", ExportValueType(env)),
        DECLARE_NAPI_PROPERTY("SyncMode", ExportSyncMode(env)),
        DECLARE_NAPI_PROPERTY("SubscribeType", ExportSubscribeType(env)),
        DECLARE_NAPI_PROPERTY("KVStoreType", ExportKVStoreType(env)),
        DECLARE_NAPI_PROPERTY("SecurityLevel", ExportSecurityLevel(env)),
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);

    return napi_define_properties(env, exports, count, properties);
}

static napi_value Init(napi_env env, napi_value exports)
{
    const napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createKVManager", JsKVManager::CreateKVManager)
    };
    napi_status status = napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    ZLOGI("init createKVManager %{public}d", status);

    status = napi_set_named_property(env, exports, "FieldNode", JsFieldNode::Constructor(env));
    ZLOGI("init FieldNode %{public}d", status);

    status = napi_set_named_property(env, exports, "Schema", JsSchema::Constructor(env));
    ZLOGI("init Schema %{public}d", status);

    status = napi_set_named_property(env, exports, "Query", JsQuery::Constructor(env));
    ZLOGI("init Query %{public}d", status);

    status = InitEnumerates(env, exports);
    ZLOGI("init Enumerate Constants %{public}d", status);
    return exports;
}

static __attribute__((constructor)) void RegisterModule()
{
    static napi_module module = { .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "data.distributedData",
        .nm_priv = ((void*)0),
        .reserved = { 0 } };
    napi_module_register(&module);
    ZLOGI("module register data.distributedData");
}

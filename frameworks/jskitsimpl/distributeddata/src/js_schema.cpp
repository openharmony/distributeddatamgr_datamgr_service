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
#define LOG_TAG "JS_Schema"
#include "js_schema.h"
#include <nlohmann/json.hpp>

#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;
using json = nlohmann::json;

namespace OHOS::DistributedData {
static std::string LABEL = "Schema";
static std::string SCHEMA_VERSION = "SCHEMA_VERSION";
static std::string SCHEMA_MODE = "SCHEMA_MODE";
static std::string SCHEMA_DEFINE = "SCHEMA_DEFINE";
static std::string SCHEMA_INDEX = "SCHEMA_INDEX";
static std::string SCHEMA_SKIPSIZE = "SCHEMA_SKIPSIZE";
static std::string DEFAULT_SCHEMA_VERSION = "1.0";

JsSchema::JsSchema(napi_env env_)
    : env(env_)
{
}

JsSchema::~JsSchema()
{
    ZLOGD("no memory leak for JsSchema");
    if (ref != nullptr) {
        napi_delete_reference(env, ref);
    }
}

napi_value JsSchema::Constructor(napi_env env)
{
    ZLOGD("Init JsSchema");
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("toJsonString", ToJson),
        DECLARE_NAPI_GETTER_SETTER("root", GetRootNode, SetRootNode)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "Schema", properties, count, JsSchema::New);
}

napi_value JsSchema::New(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::New");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    JsSchema* schema = new (std::nothrow) JsSchema(env);
    NAPI_ASSERT(env, schema !=nullptr, "no memory for schema");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("Schema finalize.");
        auto* schema = reinterpret_cast<JsSchema*>(data);
        ZLOGE_RETURN_VOID(schema != nullptr, "finalize null!");
        delete schema;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, schema, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsSchema::ToJson(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::New");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
    auto json = schema->Dump();
    JSUtil::SetValue(env, json, ctxt->output);
    return ctxt->output;
}
napi_value JsSchema::GetRootNode(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::GetRootNode");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
    NAPI_ASSERT(env, schema->ref != nullptr, "no root, please set first!");
    NAPI_CALL(env, napi_get_reference_value(env, schema->ref, &ctxt->output));
    return ctxt->output;
}
napi_value JsSchema::SetRootNode(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::SetRootNode");
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <root-node>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        JsFieldNode* node = nullptr;
        ctxt->status = JSUtil::Unwrap(env, argv[0], (void**)(&node), JsFieldNode::Constructor(env));
        ZLOGE_ON_STATUS(ctxt, "napi_unwrap to FieldNode failed");
        ZLOGE_ON_ARGS(ctxt, node != nullptr, "invalid arg[0], i.e. invalid node!");

        auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
        if (schema->ref != nullptr) {
            napi_delete_reference(env, schema->ref);
        }
        NAPI_CALL_RETURN_VOID(env, napi_create_reference(env, argv[0], 1, &schema->ref));
        schema->rootFieldNode = node;
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));
    return ctxt->self;
}
std::string JsSchema::Dump()
{
    json js = {
        { SCHEMA_VERSION, DEFAULT_SCHEMA_VERSION },
        { SCHEMA_MODE, "SCHEMA_MODE" },
        { SCHEMA_DEFINE, "SCHEMA_DEFINE" },
        { SCHEMA_INDEX, "SCHEMA_INDEX" },
        { SCHEMA_SKIPSIZE, "SCHEMA_SKIPSIZE" }
    };
    return js.dump();
}
}

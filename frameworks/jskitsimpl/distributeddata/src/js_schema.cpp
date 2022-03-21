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
static std::string SCHEMA_INDEXES = "SCHEMA_INDEXES";
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
        DECLARE_NAPI_GETTER_SETTER("root", JsSchema::GetRootNode, JsSchema::SetRootNode),
        DECLARE_NAPI_GETTER_SETTER("indexes", JsSchema::GetIndexes, JsSchema::SetIndexes),
        DECLARE_NAPI_GETTER_SETTER("mode", JsSchema::GetMode, JsSchema::SetMode),
        DECLARE_NAPI_GETTER_SETTER("skip", JsSchema::GetSkip, JsSchema::SetSkip)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "Schema", properties, count, JsSchema::New);
}

napi_value JsSchema::New(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::New");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    JsSchema* schema = new (std::nothrow) JsSchema(env);
    NAPI_ASSERT(env, schema !=nullptr, "no memory for schema");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("Schema finalize.");
        auto* schema = reinterpret_cast<JsSchema*>(data);
        CHECK_RETURN_VOID(schema != nullptr, "finalize null!");
        delete schema;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, schema, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_status JsSchema::ToJson(napi_env env, napi_value inner, JsSchema*& out)
{
    ZLOGD("Schema::ToJson");
    return JSUtil::Unwrap(env, inner, (void**)(&out), JsSchema::Constructor(env));
}

JsSchema* JsSchema::GetSchema(napi_env env, napi_callback_info info, std::shared_ptr<ContextBase>& ctxt)
{
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return reinterpret_cast<JsSchema*>(ctxt->native);
}

template <typename T>
napi_value JsSchema::GetContextValue(napi_env env, std::shared_ptr<ContextBase>& ctxt, T& value)
{
    JSUtil::SetValue(env, value, ctxt->output);
    return ctxt->output;
}

napi_value JsSchema::GetRootNode(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::GetRootNode");
    auto ctxt = std::make_shared<ContextBase>();
    auto schema = GetSchema(env, info, ctxt);
    if (schema->rootNode == nullptr) {
        int argc = 1;
        napi_value argv[1] = { nullptr };
        std::string root(SCHEMA_DEFINE);
        JSUtil::SetValue(env, root, argv[0]);
        schema->ref = JSUtil::NewWithRef(env, argc, argv,
            (void**)&schema->rootNode, JsFieldNode::Constructor(env));
    }
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
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        JsFieldNode* node = nullptr;
        ctxt->status = JSUtil::Unwrap(env, argv[0], (void**)(&node), JsFieldNode::Constructor(env));
        CHECK_STATUS_RETURN_VOID(ctxt, "napi_unwrap to FieldNode failed");
        CHECK_ARGS_RETURN_VOID(ctxt, node != nullptr, "invalid arg[0], i.e. invalid node!");

        auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
        if (schema->ref != nullptr) {
            napi_delete_reference(env, schema->ref);
        }
        ctxt->status = napi_create_reference(env, argv[0], 1, &schema->ref);
        CHECK_STATUS_RETURN_VOID(ctxt, "napi_create_reference to FieldNode failed");
        schema->rootNode = node;
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    return ctxt->self;
}

napi_value JsSchema::GetMode(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::GetMode");
    auto ctxt = std::make_shared<ContextBase>();
    auto schema = GetSchema(env, info, ctxt);
    return GetContextValue(env, ctxt, schema->mode);
}

napi_value JsSchema::SetMode(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    uint32_t mode = false;
    auto input = [env, ctxt, &mode](size_t argc, napi_value* argv) {
        // required 1 arguments :: <mode>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], mode);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid mode!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
    schema->mode = mode;
    return nullptr;
}

napi_value JsSchema::GetSkip(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::GetSkip");
    auto ctxt = std::make_shared<ContextBase>();
    auto schema = GetSchema(env, info, ctxt);
    return GetContextValue(env, ctxt, schema->skip);
}

napi_value JsSchema::SetSkip(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    uint32_t skip = false;
    auto input = [env, ctxt, &skip](size_t argc, napi_value* argv) {
        // required 1 arguments :: <skip size>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], skip);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid skip size!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
    schema->skip = skip;
    return nullptr;
}

napi_value JsSchema::GetIndexes(napi_env env, napi_callback_info info)
{
    ZLOGD("Schema::GetIndexes");
    auto ctxt = std::make_shared<ContextBase>();
    auto schema = GetSchema(env, info, ctxt);
    return GetContextValue(env, ctxt, schema->indexes);
}

napi_value JsSchema::SetIndexes(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    std::vector<std::string> indexes;
    auto input = [env, ctxt, &indexes](size_t argc, napi_value* argv) {
        // required 1 arguments :: <indexes>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], indexes);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid indexes!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto schema = reinterpret_cast<JsSchema*>(ctxt->native);
    schema->indexes = indexes;
    return nullptr;
}

std::string JsSchema::Dump()
{
    json jsIndexes = nlohmann::json::array();
    for (auto idx : indexes) {
        jsIndexes.push_back(idx);
    }
    json js = {
        { SCHEMA_VERSION, DEFAULT_SCHEMA_VERSION },
        { SCHEMA_MODE, (mode == SCHEMA_MODE_STRICT) ? "STRICT" : "COMPATIBLE" },
        { SCHEMA_DEFINE, rootNode->GetValueForJson() },
        { SCHEMA_INDEXES, jsIndexes },
        { SCHEMA_SKIPSIZE, skip },
    };
    return js.dump();
}
} // namespace OHOS::DistributedData

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
#define LOG_TAG "JS_FieldNode"
#include "js_field_node.h"
#include <nlohmann/json.hpp>

#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;
using json = nlohmann::json;

namespace OHOS::DistributedData {
static std::string FIELDNAME = "FIELDNAME";
static std::string DEFAULTVALUE = "DEFAULTVALUE";
static std::string ISWITHDEFAULTVALUE = "ISWITHDEFAULTVALUE";
static std::string CHILDREN = "CHILDREN";

JsFieldNode::JsFieldNode(const std::string& fName)
    : fieldName(fName)
{
}

napi_value JsFieldNode::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("appendChild", JsFieldNode::AppendChild),
        DECLARE_NAPI_FUNCTION("toJson", JsFieldNode::ToJson),
        DECLARE_NAPI_GETTER_SETTER("defaultValue", JsFieldNode::GetDefaultValue, JsFieldNode::SetDefaultValue)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "FieldNode", properties, count, JsFieldNode::New);
}

napi_value JsFieldNode::New(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    std::string fieldName;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &fieldName](size_t argc, napi_value* argv) {
        // required 1 arguments :: <fieldName>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], fieldName);
        ZLOGE_ON_STATUS(ctxt, "invalid arg[0], i.e. invalid fieldName!");
        ZLOGE_ON_ARGS(ctxt, !fieldName.empty(), "invalid arg[0], i.e. invalid fieldName!");
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info, input));

    JsFieldNode* fieldNode = new (std::nothrow) JsFieldNode(fieldName);
    NAPI_ASSERT(env, fieldNode !=nullptr, "no memory for fieldNode");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("fieldNode finalize.");
        auto* field = reinterpret_cast<JsFieldNode*>(data);
        ZLOGE_RETURN_VOID(field != nullptr, "finalize null!");
        delete field;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, fieldNode, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsFieldNode::AppendChild(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    JsFieldNode* child = nullptr;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &child](size_t argc, napi_value* argv) {
        // required 1 arguments :: <child>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::Unwrap(env, argv[0], (void**)(&child), JsFieldNode::Constructor(env));
        ZLOGE_ON_STATUS(ctxt, "napi_unwrap to FieldNode failed");
        ZLOGE_ON_ARGS(ctxt, child != nullptr, "invalid arg[0], i.e. invalid FieldNode!");
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info, input));

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->fields.push_back(child);

    napi_get_boolean(env, true, &ctxt->output);
    return ctxt->output;
}
napi_value JsFieldNode::ToJson(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    std::string js = fieldNode->Dump();
    JSUtil::SetValue(env, js, ctxt->output);
    return ctxt->output;
}
napi_value JsFieldNode::GetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    JSUtil::SetValue(env, fieldNode->defaultValue, ctxt->output);
    return ctxt->output;
}
napi_value JsFieldNode::SetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    auto ctxt = std::make_shared<ContextBase>();
    JSUtil::KvStoreVariant vv;
    auto input = [env, ctxt, &vv](size_t argc, napi_value* argv) {
        // required 1 arguments :: <defaultValue>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], vv);
        ZLOGE_ON_STATUS(ctxt, "invalid arg[0], i.e. invalid defaultValue!");
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info, input));

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->defaultValue = vv;
    return nullptr;
}

std::string JsFieldNode::Dump()
{
    json jsFields;
    for (auto fld : fields) {
        jsFields.push_back(fld->Dump());
    }

    auto defaultValueStr = [this]() -> std::string {
        auto strValue = std::get_if<std::string>(&defaultValue);
        if (strValue != nullptr) {
            return (*strValue);
        }
        auto intValue = std::get_if<int32_t>(&defaultValue);
        if (intValue != nullptr) {
            return std::to_string(*intValue);
        }
        auto fltValue = std::get_if<float>(&defaultValue);
        if (fltValue != nullptr) {
            return std::to_string(*fltValue);
        }
        auto boolValue = std::get_if<bool>(&defaultValue);
        if (boolValue != nullptr) {
            return std::to_string(*boolValue);
        }
        auto dblValue = std::get_if<double>(&defaultValue);
        if (dblValue != nullptr) {
            return std::to_string(*dblValue);
        }
        ZLOGE("ValueType is INVALID");
        return std::string();
    };

    json jsNode = {
        { FIELDNAME, fieldName },
        { DEFAULTVALUE, defaultValueStr() },
        { ISWITHDEFAULTVALUE, isWithDefaultValue },
        { CHILDREN, jsFields.dump() }
    };
    return jsNode.dump();
}
}

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
#define LOG_TAG "JS_FieldNode"
#include "js_field_node.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedData {
static std::string FIELDNAME = "FIELDNAME";
static std::string VALUETYPE = "VALUETYPE";
static std::string DEFAULTVALUE = "DEFAULTVALUE";
static std::string ISWITHDEFAULTVALUE = "ISWITHDEFAULTVALUE";
static std::string ISNULLABLE = "ISNULLABLE";
static std::string CHILDREN = "CHILDREN";

JsFieldNode::JsFieldNode(const std::string& fName)
    : fieldName(fName)
{
}

std::string JsFieldNode::GetFieldName()
{
    return fieldName;
}

JsFieldNode::json JsFieldNode::GetValueForJson()
{
    if (!fields.empty()) {
        /* example:
        { "field_root": {
            "field_child1": "LONG, NOT NULL, DEFAULT 88",
            "field_child2": "LONG, NOT NULL, DEFAULT 88" } } */
        json jsFields;
        for (auto fld : fields) {
            jsFields[fld->fieldName] = fld->GetValueForJson();
        }
        return jsFields;
    }

    /* example: { "field_name": "LONG, NOT NULL, DEFAULT 88" } */
    return ValueTypeToString(valueType) + "," + (isNullable ? "NULL" : "NOT NULL");
}

napi_value JsFieldNode::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("appendChild", JsFieldNode::AppendChild),
        DECLARE_NAPI_FUNCTION("toJson", JsFieldNode::ToJson),
        DECLARE_NAPI_GETTER_SETTER("default", JsFieldNode::GetDefaultValue, JsFieldNode::SetDefaultValue),
        DECLARE_NAPI_GETTER_SETTER("nullable", JsFieldNode::GetNullable, JsFieldNode::SetNullable),
        DECLARE_NAPI_GETTER_SETTER("type", JsFieldNode::GetValueType, JsFieldNode::SetValueType)
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
        CHECK_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], fieldName);
        CHECK_STATUS(ctxt, "invalid arg[0], i.e. invalid fieldName!");
        CHECK_ARGS(ctxt, !fieldName.empty(), "invalid arg[0], i.e. invalid fieldName!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    JsFieldNode* fieldNode = new (std::nothrow) JsFieldNode(fieldName);
    NAPI_ASSERT(env, fieldNode != nullptr, "no memory for fieldNode");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("fieldNode finalize.");
        auto* field = reinterpret_cast<JsFieldNode*>(data);
        CHECK_RETURN_VOID(field != nullptr, "finalize null!");
        delete field;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, fieldNode, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsFieldNode::AppendChild(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::AppendChild");
    JsFieldNode* child = nullptr;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &child](size_t argc, napi_value* argv) {
        // required 1 arguments :: <child>
        CHECK_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::Unwrap(env, argv[0], (void**)(&child), JsFieldNode::Constructor(env));
        CHECK_STATUS(ctxt, "napi_unwrap to FieldNode failed");
        CHECK_ARGS(ctxt, child != nullptr, "invalid arg[0], i.e. invalid FieldNode!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->fields.push_back(child);

    napi_get_boolean(env, true, &ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::ToJson(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::ToJson");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    std::string js = fieldNode->Dump();
    JSUtil::SetValue(env, js, ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::GetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::GetDefaultValue");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    JSUtil::SetValue(env, fieldNode->defaultValue, ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::SetDefaultValue(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::SetDefaultValue");
    auto ctxt = std::make_shared<ContextBase>();
    JSUtil::KvStoreVariant vv;
    auto input = [env, ctxt, &vv](size_t argc, napi_value* argv) {
        // required 1 arguments :: <defaultValue>
        CHECK_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], vv);
        CHECK_STATUS(ctxt, "invalid arg[0], i.e. invalid defaultValue!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->defaultValue = vv;
    return nullptr;
}

napi_value JsFieldNode::GetNullable(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::GetNullable");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    JSUtil::SetValue(env, fieldNode->isNullable, ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::SetNullable(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::SetNullable");
    auto ctxt = std::make_shared<ContextBase>();
    bool isNullable = false;
    auto input = [env, ctxt, &isNullable](size_t argc, napi_value* argv) {
        // required 1 arguments :: <isNullable>
        CHECK_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], isNullable);
        CHECK_STATUS(ctxt, "invalid arg[0], i.e. invalid isNullable!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->isNullable = isNullable;
    return nullptr;
}

napi_value JsFieldNode::GetValueType(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    JSUtil::SetValue(env, fieldNode->valueType, ctxt->output);
    return ctxt->output;
}

napi_value JsFieldNode::SetValueType(napi_env env, napi_callback_info info)
{
    ZLOGD("FieldNode::New");
    auto ctxt = std::make_shared<ContextBase>();
    uint32_t type = 0;
    auto input = [env, ctxt, &type](size_t argc, napi_value* argv) {
        // required 1 arguments :: <valueType>
        CHECK_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], type);
        CHECK_STATUS(ctxt, "invalid arg[0], i.e. invalid valueType!");
        CHECK_ARGS(ctxt, (JSUtil::STRING <= type) && (type <= JSUtil::DOUBLE),
            "invalid arg[0], i.e. invalid valueType!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto fieldNode = reinterpret_cast<JsFieldNode*>(ctxt->native);
    fieldNode->valueType = type;
    return nullptr;
}

std::string JsFieldNode::ValueToString(JSUtil::KvStoreVariant value)
{
    auto strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr) {
        return (*strValue);
    }
    auto intValue = std::get_if<int32_t>(&value);
    if (intValue != nullptr) {
        return std::to_string(*intValue);
    }
    auto fltValue = std::get_if<float>(&value);
    if (fltValue != nullptr) {
        return std::to_string(*fltValue);
    }
    auto boolValue = std::get_if<bool>(&value);
    if (boolValue != nullptr) {
        return std::to_string(*boolValue);
    }
    auto dblValue = std::get_if<double>(&value);
    if (dblValue != nullptr) {
        return std::to_string(*dblValue);
    }
    ZLOGE("ValueType is INVALID");
    return std::string();
}

std::string JsFieldNode::ValueTypeToString(uint32_t type)
{
    // DistributedDB::FieldType
    switch (type) {
        case JSUtil::STRING:
            return std::string("STRING");
        case JSUtil::INTEGER:
            return std::string("INTEGER");
        case JSUtil::FLOAT:
            return std::string("FLOAT");
        case JSUtil::BYTE_ARRAY:
            return std::string("BYTE_ARRAY");
        case JSUtil::BOOLEAN:
            return std::string("BOOLEAN");
        case JSUtil::DOUBLE:
            return std::string("DOUBLE");
        default:
            ZLOGE("ValueType is INVALID");
            break;
    }
    return std::string();
}
std::string JsFieldNode::Dump()
{
    json jsFields;
    for (auto fld : fields) {
        jsFields.push_back(fld->Dump());
    }

    json jsNode = {
        { FIELDNAME, fieldName },
        { VALUETYPE, ValueTypeToString(valueType) },
        { DEFAULTVALUE, ValueToString(defaultValue) },
        { ISWITHDEFAULTVALUE, isWithDefaultValue },
        { ISNULLABLE, isNullable },
        { CHILDREN, jsFields.dump() }
    };
    return jsNode.dump();
}
}

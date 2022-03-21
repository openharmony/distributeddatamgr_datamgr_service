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
#define LOG_TAG "JS_Query"
#include "js_query.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedData {
DataQuery& JsQuery::GetNative()
{
    return query_;
}

napi_value JsQuery::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("reset", JsQuery::Reset),
        DECLARE_NAPI_FUNCTION("equalTo", JsQuery::EqualTo),
        DECLARE_NAPI_FUNCTION("notEqualTo", JsQuery::NotEqualTo),
        DECLARE_NAPI_FUNCTION("greaterThan", JsQuery::GreaterThan),
        DECLARE_NAPI_FUNCTION("lessThan", JsQuery::LessThan),
        DECLARE_NAPI_FUNCTION("greaterThanOrEqualTo", JsQuery::GreaterThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("lessThanOrEqualTo", JsQuery::LessThanOrEqualTo),
        DECLARE_NAPI_FUNCTION("isNull", JsQuery::IsNull),
        DECLARE_NAPI_FUNCTION("inNumber", JsQuery::InNumber),
        DECLARE_NAPI_FUNCTION("inString", JsQuery::InString),
        DECLARE_NAPI_FUNCTION("notInNumber", JsQuery::NotInNumber),
        DECLARE_NAPI_FUNCTION("notInString", JsQuery::NotInString),
        DECLARE_NAPI_FUNCTION("like", JsQuery::Like),
        DECLARE_NAPI_FUNCTION("unlike", JsQuery::Unlike),
        DECLARE_NAPI_FUNCTION("and", JsQuery::And),
        DECLARE_NAPI_FUNCTION("or", JsQuery::Or),
        DECLARE_NAPI_FUNCTION("orderByAsc", JsQuery::OrderByAsc),
        DECLARE_NAPI_FUNCTION("orderByDesc", JsQuery::OrderByDesc),
        DECLARE_NAPI_FUNCTION("limit", JsQuery::Limit),
        DECLARE_NAPI_FUNCTION("isNotNull", JsQuery::IsNotNull),
        DECLARE_NAPI_FUNCTION("beginGroup", JsQuery::BeginGroup),
        DECLARE_NAPI_FUNCTION("endGroup", JsQuery::EndGroup),
        DECLARE_NAPI_FUNCTION("prefixKey", JsQuery::PrefixKey),
        DECLARE_NAPI_FUNCTION("setSuggestIndex", JsQuery::SetSuggestIndex),
        DECLARE_NAPI_FUNCTION("deviceId", JsQuery::DeviceId),
        DECLARE_NAPI_FUNCTION("getSqlLike", JsQuery::GetSqlLike)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "Query", properties, count, JsQuery::New);
}

/*
 * [JS API Prototype]
 *      var query = new ddm.JsQuery();
 */
napi_value JsQuery::New(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    JsQuery* query = new (std::nothrow) JsQuery();
    NAPI_ASSERT(env, query !=nullptr, "no memory for query");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("query finalize.");
        auto* query = reinterpret_cast<JsQuery*>(data);
        CHECK_RETURN_VOID(query != nullptr, "finalize null!");
        delete query;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, query, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsQuery::Reset(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Reset()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.Reset();
    return ctxt->self;
}

struct ValueContext : public ContextBase {
    std::string field;
    JSUtil::QueryVariant vv;

    void GetValueSync(napi_env env, napi_callback_info info)
    {
        auto input = [this, env](size_t argc, napi_value* argv) {
            // required 2 arguments :: <field> <value>
            CHECK_ARGS_RETURN_VOID(this, argc == 2, "invalid arguments!");
            status = JSUtil::GetValue(env, argv[0], field);
            CHECK_STATUS_RETURN_VOID(this, "invalid arg[0], i.e. invalid field!");
            status = JSUtil::GetValue(env, argv[1], vv);
            CHECK_STATUS_RETURN_VOID(this, "invalid arg[1], i.e. invalid value!");
        };
        GetCbInfoSync(env, info, input);
    }
};

/* [js] equalTo(field:string, value:number|string|boolean):JsQuery */
napi_value JsQuery::EqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::EqualTo()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.EqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.EqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.EqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::NotEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::NotEqualTo()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.NotEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.NotEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.NotEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::GreaterThan(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::GreaterThan()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.GreaterThan(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.GreaterThan(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.GreaterThan(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::LessThan(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::LessThan()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.LessThan(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.LessThan(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.LessThan(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::GreaterThanOrEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::GreaterThanOrEqualTo()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.GreaterThanOrEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.GreaterThanOrEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.GreaterThanOrEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::LessThanOrEqualTo(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::LessThanOrEqualTo()");
    auto ctxt = std::make_shared<ValueContext>();
    ctxt->GetValueSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    auto strValue = std::get_if<std::string>(&ctxt->vv);
    if (strValue != nullptr) {
        query.LessThanOrEqualTo(ctxt->field, *strValue);
    } else {
        auto boolValue = std::get_if<bool>(&ctxt->vv);
        if (boolValue != nullptr) {
            query.LessThanOrEqualTo(ctxt->field, *boolValue);
        } else {
            auto dblValue = std::get_if<double>(&ctxt->vv);
            if (dblValue != nullptr) {
                query.LessThanOrEqualTo(ctxt->field, *dblValue);
            }
        }
    }
    return ctxt->self;
}

napi_value JsQuery::IsNull(napi_env env, napi_callback_info info)
{
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.IsNull(field);
    return ctxt->self;
}

/*
 * InNumber / NotInNumber
 * [NOTES] Recommented to use the napi_typedarray_type
 * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects#indexed_collections
 */
enum class NumberType : uint8_t {
    NUMBER_INT,
    NUMBER_LONG,
    NUMBER_DOUBLE,
    NUMBER_INVALID = 255
};
struct NumbersContext : public ContextBase {
    std::string field;
    std::vector<int> intList;
    std::vector<int64_t> longList;
    std::vector<double> doubleList;
    NumberType innerType = NumberType::NUMBER_INVALID;

    void GetNumberSync(napi_env env, napi_callback_info info)
    {
        auto input = [this, env](size_t argc, napi_value* argv) {
            // required 2 arguments :: <field> <value-list>
            CHECK_ARGS_RETURN_VOID(this, argc == 2, "invalid arguments!");
            status = JSUtil::GetValue(env, argv[0], field);
            CHECK_STATUS_RETURN_VOID(this, "invalid arg[0], i.e. invalid field!");

            bool isTypedArray = false;
            status = napi_is_typedarray(env, argv[1], &isTypedArray);
            ZLOGD("arg[1] %{public}s a TypedArray", isTypedArray ? "is" : "is not");
            if (isTypedArray && (status == napi_ok)) {
                napi_typedarray_type type = napi_biguint64_array;
                size_t length = 0;
                napi_value buffer = nullptr;
                size_t offset = 0;
                void* data = nullptr;
                status = napi_get_typedarray_info(env, argv[1], &type, &length, &data, &buffer, &offset);
                CHECK_STATUS_RETURN_VOID(this, "invalid arg[1], i.e. invalid number array!");
                if (type < napi_uint32_array) {
                    status = JSUtil::GetValue(env, argv[1], intList);
                    innerType = NumberType::NUMBER_INT;
                } else if (type == napi_bigint64_array || type == napi_uint32_array) {
                    status = JSUtil::GetValue(env, argv[1], longList);
                    innerType = NumberType::NUMBER_LONG;
                } else {
                    status = JSUtil::GetValue(env, argv[1], doubleList);
                    innerType = NumberType::NUMBER_DOUBLE;
                }
            } else {
                bool isArray = false;
                status = napi_is_array(env, argv[1], &isArray);
                CHECK_ARGS_RETURN_VOID(this, isArray, "invalid arg[1], i.e. invalid number array!");
                ZLOGD("arg[1] %{public}s a Array, treat as array of double.", isTypedArray ? "is" : "is not");
                status = JSUtil::GetValue(env, argv[1], doubleList);
                CHECK_STATUS_RETURN_VOID(this, "invalid arg[1], i.e. invalid number array!");
                innerType = NumberType::NUMBER_DOUBLE;
            };
        };
        GetCbInfoSync(env, info, input);
    }
};

napi_value JsQuery::InNumber(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<NumbersContext>();
    ctxt->GetNumberSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    if (ctxt->innerType == NumberType::NUMBER_INT) {
        query.InInt(ctxt->field, ctxt->intList);
    } else if (ctxt->innerType == NumberType::NUMBER_LONG) {
        query.InLong(ctxt->field, ctxt->longList);
    } else if (ctxt->innerType == NumberType::NUMBER_DOUBLE) {
        query.InDouble(ctxt->field, ctxt->doubleList);
    }
    return ctxt->self;
}

napi_value JsQuery::InString(napi_env env, napi_callback_info info)
{
    struct StringsContext : public ContextBase {
        std::string field;
        std::vector<std::string> valueList;
    };
    auto ctxt = std::make_shared<StringsContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <valueList>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->valueList);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid valueList!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.InString(ctxt->field, ctxt->valueList);
    return ctxt->self;
}

napi_value JsQuery::NotInNumber(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<NumbersContext>();
    ctxt->GetNumberSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    if (ctxt->innerType == NumberType::NUMBER_INT) {
        query.NotInInt(ctxt->field, ctxt->intList);
    } else if (ctxt->innerType == NumberType::NUMBER_LONG) {
        query.NotInLong(ctxt->field, ctxt->longList);
    } else if (ctxt->innerType == NumberType::NUMBER_DOUBLE) {
        query.NotInDouble(ctxt->field, ctxt->doubleList);
    }
    return ctxt->self;
}

napi_value JsQuery::NotInString(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::NotInString()");
    struct StringsContext : public ContextBase {
        std::string field;
        std::vector<std::string> valueList;
    };
    auto ctxt = std::make_shared<StringsContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <valueList>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->valueList);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid valueList!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.NotInString(ctxt->field, ctxt->valueList);
    return ctxt->self;
}

napi_value JsQuery::Like(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Like()");
    struct LikeContext : public ContextBase {
        std::string field;
        std::string value;
    };
    auto ctxt = std::make_shared<LikeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <value>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->value);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid value!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.Like(ctxt->field, ctxt->value);
    return ctxt->self;
}

napi_value JsQuery::Unlike(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Unlike()");
    struct UnlikeContext : public ContextBase {
        std::string field;
        std::string value;
    };
    auto ctxt = std::make_shared<UnlikeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <field> <value>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->value);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid value!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.Unlike(ctxt->field, ctxt->value);
    return ctxt->self;
}

napi_value JsQuery::And(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::And()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.And();
    return ctxt->self;
}

napi_value JsQuery::Or(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::Or()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.Or();
    return ctxt->self;
}

napi_value JsQuery::OrderByAsc(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::OrderByAsc()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.OrderByAsc(field);
    return ctxt->self;
}

napi_value JsQuery::OrderByDesc(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::OrderByAsc()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.OrderByDesc(field);
    return ctxt->self;
}

napi_value JsQuery::Limit(napi_env env, napi_callback_info info)
{
    struct LimitContext : public ContextBase {
        int number;
        int offset;
    };
    auto ctxt = std::make_shared<LimitContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <number> <offset>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 2, "invalid arguments!");
        ctxt->status = napi_get_value_int32(env, argv[0], &ctxt->number);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid number!");
        ctxt->status = napi_get_value_int32(env, argv[1], &ctxt->offset);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[1], i.e. invalid offset!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.Limit(ctxt->number, ctxt->offset);
    return ctxt->self;
}

napi_value JsQuery::IsNotNull(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::IsNotNull()");
    std::string field;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &field](size_t argc, napi_value* argv) {
        // required 1 arguments :: <field>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], field);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid field!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.IsNotNull(field);
    return ctxt->self;
}

napi_value JsQuery::BeginGroup(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::BeginGroup()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");
    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.BeginGroup();
    return ctxt->self;
}

napi_value JsQuery::EndGroup(napi_env env, napi_callback_info info)
{
    ZLOGD("Query::EndGroup()");
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.EndGroup();
    return ctxt->self;
}

napi_value JsQuery::PrefixKey(napi_env env, napi_callback_info info)
{
    std::string prefix;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &prefix](size_t argc, napi_value* argv) {
        // required 1 arguments :: <prefix>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], prefix);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid prefix!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.KeyPrefix(prefix);
    return ctxt->self;
}

napi_value JsQuery::SetSuggestIndex(napi_env env, napi_callback_info info)
{
    std::string suggestIndex;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &suggestIndex](size_t argc, napi_value* argv) {
        // required 1 arguments :: <suggestIndex>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], suggestIndex);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid suggestIndex!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.SetSuggestIndex(suggestIndex);
    return ctxt->self;
}

napi_value JsQuery::DeviceId(napi_env env, napi_callback_info info)
{
    std::string deviceId;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &deviceId](size_t argc, napi_value* argv) {
        // required 1 arguments :: <deviceId>
        CHECK_ARGS_RETURN_VOID(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = JSUtil::GetValue(env, argv[0], deviceId);
        CHECK_STATUS_RETURN_VOID(ctxt, "invalid arg[0], i.e. invalid deviceId!");
    };
    ctxt->GetCbInfoSync(env, info, input);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    query.DeviceId(deviceId);
    return ctxt->self;
}

// getSqlLike():string
napi_value JsQuery::GetSqlLike(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfoSync(env, info);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "invalid arguments!");

    auto& query = reinterpret_cast<JsQuery*>(ctxt->native)->query_;
    JSUtil::SetValue(env, query.ToString(), ctxt->output);
    return ctxt->output;
}
} // namespace OHOS::DistributedData

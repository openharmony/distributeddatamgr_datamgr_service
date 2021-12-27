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
#define LOG_TAG "JS_KVStoreResultSet"
#include "js_kv_store_resultset.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

using namespace OHOS::DistributedKv;
namespace OHOS::DistributedData {
void JsKVStoreResultSet::SetNative(std::shared_ptr<KvStoreResultSet>& resultSet)
{
    resultSet_ = resultSet;
}

std::shared_ptr<KvStoreResultSet>& JsKVStoreResultSet::GetNative()
{
    return resultSet_;
}

napi_value JsKVStoreResultSet::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getCount", JsKVStoreResultSet::GetCount),
        DECLARE_NAPI_FUNCTION("getPosition", JsKVStoreResultSet::GetPosition),
        DECLARE_NAPI_FUNCTION("moveToFirst", JsKVStoreResultSet::MoveToFirst),
        DECLARE_NAPI_FUNCTION("moveToLast", JsKVStoreResultSet::MoveToLast),
        DECLARE_NAPI_FUNCTION("moveToNext", JsKVStoreResultSet::MoveToNext),
        DECLARE_NAPI_FUNCTION("moveToPrevious", JsKVStoreResultSet::MoveToPrevious),
        DECLARE_NAPI_FUNCTION("move", JsKVStoreResultSet::Move),
        DECLARE_NAPI_FUNCTION("moveToPosition", JsKVStoreResultSet::MoveToPosition),
        DECLARE_NAPI_FUNCTION("isFirst", JsKVStoreResultSet::IsFirst),
        DECLARE_NAPI_FUNCTION("isLast", JsKVStoreResultSet::IsLast),
        DECLARE_NAPI_FUNCTION("isBeforeFirst", JsKVStoreResultSet::IsBeforeFirst),
        DECLARE_NAPI_FUNCTION("isAfterLast", JsKVStoreResultSet::IsAfterLast),
        DECLARE_NAPI_FUNCTION("getEntry", JsKVStoreResultSet::GetEntry)
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "KVStoreResultSet", properties, count, JsKVStoreResultSet::New);
}

napi_value JsKVStoreResultSet::New(napi_env env, napi_callback_info info)
{
    ZLOGD("constructor JsKVStoreResultSet!");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    JsKVStoreResultSet* resultSet = new (std::nothrow) JsKVStoreResultSet();
    NAPI_ASSERT(env, resultSet !=nullptr, "no memory for resultSet");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("kvStoreResultSet finalize.");
        auto* resultSet = reinterpret_cast<JsKVStoreResultSet*>(data);
        ZLOGE_RETURN_VOID(resultSet != nullptr, "finalize null!");
        delete resultSet;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, resultSet, finalize, nullptr, nullptr));
    return ctxt->self;
}

napi_value JsKVStoreResultSet::GetCount(napi_env env, napi_callback_info info) /* number  */
{
    ZLOGD("KVStoreResultSet::GetCount()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));
    ZLOGD("KVStoreResultSet::GetCount(status=%{public}d)", ctxt->status);

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    int count = resultSet->GetCount();

    napi_create_int32(env, count, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::GetPosition(napi_env env, napi_callback_info info) /* number */
{
    ZLOGD("KVStoreResultSet::GetPosition()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    int position = resultSet->GetPosition();

    napi_create_int32(env, position, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::MoveToFirst(napi_env env, napi_callback_info info) /* boolean  */
{
    ZLOGD("KVStoreResultSet::MoveToFirst()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->MoveToFirst();

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::MoveToLast(napi_env env, napi_callback_info info) /* boolean */
{
    ZLOGD("KVStoreResultSet::MoveToFirst()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->MoveToLast();

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::MoveToNext(napi_env env, napi_callback_info info) /* boolean */
{
    ZLOGD("KVStoreResultSet::MoveToNext()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->MoveToNext();

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::MoveToPrevious(napi_env env, napi_callback_info info) /* boolean */
{
    ZLOGD("KVStoreResultSet::MoveToPrevious()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->MoveToPrevious();

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::Move(napi_env env, napi_callback_info info) /* boolean  */
{
    ZLOGD("KVStoreResultSet::Move()");
    int offset = 0;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &offset](size_t argc, napi_value* argv) {
        // required 1 arguments :: <offset>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = napi_get_value_int32(env, argv[0], (int32_t*)&offset);
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info, input));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->Move(offset);

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::MoveToPosition(napi_env env, napi_callback_info info) /* boolean  */
{
    int position = 0;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &position](size_t argc, napi_value* argv) {
        // required 1 arguments :: <position>
        ZLOGE_ON_ARGS(ctxt, argc == 1, "invalid arguments!");
        ctxt->status = napi_get_value_int32(env, argv[0], (int32_t*)&position);
    };
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info, input));
    ZLOGD("KVStoreResultSet::MoveToPosition(%{public}d)", position);

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isMoved = resultSet->MoveToPosition(position);

    napi_get_boolean(env, isMoved, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::IsFirst(napi_env env, napi_callback_info info) /* boolean  */
{
    ZLOGD("KVStoreResultSet::IsFirst()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isFirst = resultSet->IsFirst();

    napi_get_boolean(env, isFirst, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::IsLast(napi_env env, napi_callback_info info) /* boolean */
{
    ZLOGD("KVStoreResultSet::IsLast()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isLast = resultSet->IsLast();

    napi_get_boolean(env, isLast, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::IsBeforeFirst(napi_env env, napi_callback_info info) /* boolean  */
{
    ZLOGD("KVStoreResultSet::IsBeforeFirst()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isBeforeFirst = resultSet->IsBeforeFirst();

    napi_get_boolean(env, isBeforeFirst, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::IsAfterLast(napi_env env, napi_callback_info info) /* boolean  */
{
    ZLOGD("KVStoreResultSet::IsAfterLast()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    bool isAfterLast = resultSet->IsAfterLast();

    napi_get_boolean(env, isAfterLast, &ctxt->output);
    return ctxt->output;
}

napi_value JsKVStoreResultSet::GetEntry(napi_env env, napi_callback_info info) /* Entry */
{
    ZLOGD("KVStoreResultSet::GetEntry()");
    auto ctxt = std::make_shared<ContextBase>();
    NAPI_CALL(env, ctxt->GetCbInfoSync(env, info));

    DistributedKv::Entry entry;
    auto& resultSet = reinterpret_cast<JsKVStoreResultSet*>(ctxt->native)->resultSet_;
    auto status = resultSet->GetEntry(entry);
    if (status != Status::SUCCESS) {
        return nullptr;
    }

    ctxt->status = JSUtil::SetValue(env, entry, ctxt->output);
    NAPI_ASSERT(env, ctxt->status == napi_ok, "GetEntry failed!");
    return ctxt->output;
}
} //
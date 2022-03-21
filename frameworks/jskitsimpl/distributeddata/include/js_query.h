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
#ifndef OHOS_QUERY_H
#define OHOS_QUERY_H
#include <string>

#include "data_query.h"
#include "napi_queue.h"

namespace OHOS::DistributedData {
class JsQuery {
public:
    JsQuery() = default;
    ~JsQuery() = default;

    DistributedKv::DataQuery& GetNative();

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);

private:
    static napi_value Reset(napi_env env, napi_callback_info info);
    static napi_value EqualTo(napi_env env, napi_callback_info info);
    static napi_value NotEqualTo(napi_env env, napi_callback_info info);
    static napi_value GreaterThan(napi_env env, napi_callback_info info);
    static napi_value LessThan(napi_env env, napi_callback_info info);
    static napi_value GreaterThanOrEqualTo(napi_env env, napi_callback_info info);
    static napi_value LessThanOrEqualTo(napi_env env, napi_callback_info info);
    static napi_value IsNull(napi_env env, napi_callback_info info);
    static napi_value InNumber(napi_env env, napi_callback_info info);
    static napi_value InString(napi_env env, napi_callback_info info);
    static napi_value NotInNumber(napi_env env, napi_callback_info info);
    static napi_value NotInString(napi_env env, napi_callback_info info);
    static napi_value Like(napi_env env, napi_callback_info info);
    static napi_value Unlike(napi_env env, napi_callback_info info);
    static napi_value And(napi_env env, napi_callback_info info);
    static napi_value Or(napi_env env, napi_callback_info info);
    static napi_value OrderByAsc(napi_env env, napi_callback_info info);
    static napi_value OrderByDesc(napi_env env, napi_callback_info info);
    static napi_value Limit(napi_env env, napi_callback_info info);
    static napi_value IsNotNull(napi_env env, napi_callback_info info);
    static napi_value BeginGroup(napi_env env, napi_callback_info info);
    static napi_value EndGroup(napi_env env, napi_callback_info info);
    static napi_value PrefixKey(napi_env env, napi_callback_info info);
    static napi_value SetSuggestIndex(napi_env env, napi_callback_info info);
    static napi_value DeviceId(napi_env env, napi_callback_info info);
    static napi_value GetSqlLike(napi_env env, napi_callback_info info);

private:
    DistributedKv::DataQuery query_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_QUERY_H

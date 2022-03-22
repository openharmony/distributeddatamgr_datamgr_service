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
#ifndef OHOS_SCHEMA_H
#define OHOS_SCHEMA_H
#include <string>

#include "js_util.h"
#include "js_field_node.h"
#include "napi_queue.h"

namespace OHOS::DistributedData {
class JsSchema {
public:
    explicit JsSchema(napi_env env);
    ~JsSchema();

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);
    
    static napi_status ToJson(napi_env env, napi_value inner, JsSchema*& out);
    std::string Dump();

private:
    static napi_value GetRootNode(napi_env env, napi_callback_info info);
    static napi_value SetRootNode(napi_env env, napi_callback_info info);
    static napi_value GetMode(napi_env env, napi_callback_info info);
    static napi_value SetMode(napi_env env, napi_callback_info info);
    static napi_value GetSkip(napi_env env, napi_callback_info info);
    static napi_value SetSkip(napi_env env, napi_callback_info info);
    static napi_value GetIndexes(napi_env env, napi_callback_info info);
    static napi_value SetIndexes(napi_env env, napi_callback_info info);

    template <typename T>
    static napi_value GetContextValue(napi_env env, std::shared_ptr<ContextBase>& ctxt, T &value);
    
    static JsSchema* GetSchema(napi_env env, napi_callback_info info, std::shared_ptr<ContextBase> &ctxt);

    enum {
        SCHEMA_MODE_SLOPPY,
        SCHEMA_MODE_STRICT,
    };
    JsFieldNode* rootNode = nullptr;
    napi_env env = nullptr;     // manage the root. set/get.
    napi_ref ref = nullptr;     // manage the root. set/get.

    std::vector<std::string> indexes;
    uint32_t mode = SCHEMA_MODE_SLOPPY;
    uint32_t skip = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_SCHEMA_H

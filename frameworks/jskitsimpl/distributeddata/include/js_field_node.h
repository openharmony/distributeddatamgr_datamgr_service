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
#ifndef OHOS_FIELD_NODE_H
#define OHOS_FIELD_NODE_H
#include <string>
#include <list>

#include "js_util.h"
#include "napi_queue.h"

namespace OHOS::DistributedData {
class JsFieldNode {
public:
    JsFieldNode(const std::string& fName);
    ~JsFieldNode() = default;

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);

private:
    static napi_value AppendChild(napi_env env, napi_callback_info info);
    static napi_value ToJson(napi_env env, napi_callback_info info);
    static napi_value GetDefaultValue(napi_env env, napi_callback_info info);
    static napi_value SetDefaultValue(napi_env env, napi_callback_info info);

    std::string Dump();

    std::list<JsFieldNode*> fields;
    std::string fieldName;
    JSUtil::KvStoreVariant defaultValue;
    bool isWithDefaultValue = false;
};
}
#endif // OHOS_FIELD_NODE_H

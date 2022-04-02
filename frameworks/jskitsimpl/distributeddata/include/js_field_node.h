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
#ifndef OHOS_FIELD_NODE_H
#define OHOS_FIELD_NODE_H
#include <nlohmann/json.hpp>
#include <list>
#include "js_util.h"
#include "napi_queue.h"

namespace OHOS::DistributedData {
class JsFieldNode {
public:
    using json = nlohmann::json;
    explicit JsFieldNode(const std::string& fName);
    ~JsFieldNode() = default;

    std::string GetFieldName();
    std::string Dump();
    json GetValueForJson();

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);

private:
    static napi_value AppendChild(napi_env env, napi_callback_info info);
    static napi_value GetDefaultValue(napi_env env, napi_callback_info info);
    static napi_value SetDefaultValue(napi_env env, napi_callback_info info);
    static napi_value GetNullable(napi_env env, napi_callback_info info);
    static napi_value SetNullable(napi_env env, napi_callback_info info);
    static napi_value GetValueType(napi_env env, napi_callback_info info);
    static napi_value SetValueType(napi_env env, napi_callback_info info);
    static std::map<uint32_t, std::string> valueTypeToString_;
    
    template <typename T>
    static napi_value GetContextValue(napi_env env, std::shared_ptr<ContextBase> &ctxt, T &value);
    static JsFieldNode* GetFieldNode(napi_env env, napi_callback_info info, std::shared_ptr<ContextBase> &ctxt);
    
    std::string ValueToString(JSUtil::KvStoreVariant value);
    std::string ValueTypeToString(uint32_t type);

    std::list<JsFieldNode*> fields;
    std::string fieldName;
    uint32_t valueType = JSUtil::INVALID;
    JSUtil::KvStoreVariant defaultValue;
    bool isWithDefaultValue = false;
    bool isNullable = false;
};
} // namespace OHOS::DistributedData
#endif // OHOS_FIELD_NODE_H

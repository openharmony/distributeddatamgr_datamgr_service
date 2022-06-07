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
#ifndef OHOS_JS_UTIL_H
#define OHOS_JS_UTIL_H
#include <cstdint>
#include <map>
#include <variant>
#include "data_query.h"
#include "change_notification.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "datashare_abs_predicates.h"
#include "datashare_values_bucket.h"

namespace OHOS::DistributedData {
class JSUtil final {
public:
    enum {
        /* Blob's first byte is the blob's data ValueType */
        STRING = 0,
        INTEGER = 1,
        FLOAT = 2,
        BYTE_ARRAY = 3,
        BOOLEAN = 4,
        DOUBLE = 5,
        INVALID = 255
    };
    using JsSchema = class JsSchema;
    using Blob = OHOS::DistributedKv::Blob;
    using ChangeNotification = OHOS::DistributedKv::ChangeNotification;
    using Options = OHOS::DistributedKv::Options;
    using Entry = OHOS::DistributedKv::Entry;
    using StoreId = OHOS::DistributedKv::StoreId;
    using Status = OHOS::DistributedKv::Status;
    using DataQuery = OHOS::DistributedKv::DataQuery;
    using ValuesBucket = OHOS::DataShare::DataShareValuesBucket;
    using ValueObject = OHOS::DataShare::DataShareValueObject;
    /* for kvStore Put/Get : boolean|string|number|Uint8Array */
    using KvStoreVariant = std::variant<std::string, int32_t, float, std::vector<uint8_t>, bool, double>;
    static KvStoreVariant Blob2VariantValue(const Blob& blob);
    static Blob VariantValue2Blob(const KvStoreVariant& value);

    /* for query value related : number|string|boolean */
    using QueryVariant = std::variant<std::string, bool, double>;

    /* napi_value <-> bool */
    static napi_status GetValue(napi_env env, napi_value in, bool& out);
    static napi_status SetValue(napi_env env, const bool& in, napi_value& out);

    /* napi_value <-> int32_t */
    static napi_status GetValue(napi_env env, napi_value in, int32_t& out);
    static napi_status SetValue(napi_env env, const int32_t& in, napi_value& out);

    /* napi_value <-> uint32_t */
    static napi_status GetValue(napi_env env, napi_value in, uint32_t& out);
    static napi_status SetValue(napi_env env, const uint32_t& in, napi_value& out);

    /* napi_value <-> int64_t */
    static napi_status GetValue(napi_env env, napi_value in, int64_t& out);
    static napi_status SetValue(napi_env env, const int64_t& in, napi_value& out);

    /* napi_value <-> double */
    static napi_status GetValue(napi_env env, napi_value in, double& out);
    static napi_status SetValue(napi_env env, const double& in, napi_value& out);

    /* napi_value <-> std::string */
    static napi_status GetValue(napi_env env, napi_value in, std::string& out);
    static napi_status SetValue(napi_env env, const std::string& in, napi_value& out);

    /* napi_value <-> KvStoreVariant */
    static napi_status GetValue(napi_env env, napi_value in, KvStoreVariant& out);
    static napi_status SetValue(napi_env env, const KvStoreVariant& in, napi_value& out);

    /* napi_value <-> QueryVariant */
    static napi_status GetValue(napi_env env, napi_value in, QueryVariant& out);
    static napi_status SetValue(napi_env env, const QueryVariant& in, napi_value& out);

    /* napi_value <-> std::vector<std::string> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<std::string>& out);
    static napi_status SetValue(napi_env env, const std::vector<std::string>& in, napi_value& out);

    /* napi_value <-> std::vector<uint8_t> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<uint8_t>& out);
    static napi_status SetValue(napi_env env, const std::vector<uint8_t>& in, napi_value& out);

    /* napi_value <-> std::vector<int32_t> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<int32_t>& out);
    static napi_status SetValue(napi_env env, const std::vector<int32_t>& in, napi_value& out);

    /* napi_value <-> std::vector<uint32_t> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<uint32_t>& out);
    static napi_status SetValue(napi_env env, const std::vector<uint32_t>& in, napi_value& out);

    /* napi_value <-> std::vector<int64_t> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<int64_t>& out);
    static napi_status SetValue(napi_env env, const std::vector<int64_t>& in, napi_value& out);

    /* napi_value <-> std::vector<double> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<double>& out);
    static napi_status SetValue(napi_env env, const std::vector<double>& in, napi_value& out);

    /* napi_value <-> ChangeNotification */
    static napi_status GetValue(napi_env env, napi_value in, ChangeNotification& out);
    static napi_status SetValue(napi_env env, const ChangeNotification& in, napi_value& out);

    /* napi_value <-> Options */
    static napi_status GetValue(napi_env env, napi_value in, Options& out);
    static napi_status SetValue(napi_env env, const Options& in, napi_value& out);

    /* napi_value <-> Entry */
    static napi_status GetValue(napi_env env, napi_value in, Entry& out);
    static napi_status SetValue(napi_env env, const Entry& in, napi_value& out);

    /* napi_value <-> Options */
    static napi_status GetValue(napi_env env, napi_value in, std::list<Entry>& out);
    static napi_status SetValue(napi_env env, const std::list<Entry>& in, napi_value& out);

    /* napi_value <-> std::vector<Entry> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<Entry>& out);
    static napi_status SetValue(napi_env env, const std::vector<Entry>& in, napi_value& out);

    /* napi_value <-> std::vector<StoreId> */
    static napi_status GetValue(napi_env env, napi_value in, std::vector<StoreId>& out);
    static napi_status SetValue(napi_env env, const std::vector<StoreId>& in, napi_value& out);

    /* napi_value <-> std::map<std::string, Status> */
    static napi_status GetValue(napi_env env, napi_value in, std::map<std::string, Status>& out);
    static napi_status SetValue(napi_env env, const std::map<std::string, Status>& in, napi_value& out);
    
    static napi_status GetValue(napi_env env, napi_value in, JsSchema*& out);

    static napi_status GetValue(napi_env env, napi_value in, std::vector<Blob> &out);
    static napi_status GetValue(napi_env env, napi_value in, DataQuery &out);

    static napi_status GetValue(napi_env env, napi_value jsValue, ValueObject &valueObject);
    static napi_status GetValue(napi_env env, napi_value jsValue, ValuesBucket &valuesBucket);

    /* napi_get_named_property wrapper */
    template <typename T>
    static inline napi_status GetNamedProperty(napi_env env, napi_value in, const std::string& prop, T& value)
    {
        bool hasProp = false;
        napi_status status = napi_has_named_property(env, in, prop.c_str(), &hasProp);
        if ((status == napi_ok) && hasProp) {
            napi_value inner = nullptr;
            status = napi_get_named_property(env, in, prop.c_str(), &inner);
            if ((status == napi_ok) && (inner != nullptr)) {
                return GetValue(env, inner, value);
            }
        }
        return napi_invalid_arg;
    };

    /* napi_define_class  wrapper */
    static napi_value DefineClass(napi_env env, const std::string& name,
        const napi_property_descriptor* properties, size_t count, napi_callback newcb);

    /* napi_new_instance  wrapper */
    static napi_ref NewWithRef(napi_env env, size_t argc, napi_value* argv, void** out, napi_value constructor);

    /* napi_unwrap with napi_instanceof */
    static napi_status Unwrap(napi_env env, napi_value in, void** out, napi_value constructor);

    static bool Equals(napi_env env, napi_value value, napi_ref copy);

private:
    enum {
        /* std::map<key, value> to js::tuple<key, value> */
        TUPLE_KEY = 0,
        TUPLE_VALUE,
        TUPLE_SIZE
    };
};
} // namespace OHOS::DistributedData
#endif // OHOS_JS_UTIL_H

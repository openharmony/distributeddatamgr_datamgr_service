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
#ifndef OHOS_NAPI_QUEUE_H
#define OHOS_NAPI_QUEUE_H
#include <functional>

#include "log_print.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv; // for ZLOGD/ZLOGE
constexpr size_t ARGC_MAX = 6;

using NapiCbInfoParser = std::function<void(size_t argc, napi_value* argv)>;
using NapiAsyncExecute = std::function<void(void)>;
using NapiAsyncComplete = std::function<void(napi_value&)>;

struct ContextBase {
    virtual ~ContextBase();
    void GetCbInfo(
        napi_env env, napi_callback_info info, NapiCbInfoParser parse = NapiCbInfoParser(), bool sync = false);

    inline void GetCbInfoSync(napi_env env, napi_callback_info info, NapiCbInfoParser parse = NapiCbInfoParser())
    {
        /* sync = true, means no callback, not AsyncWork. */
        GetCbInfo(env, info, parse, true);
    }

    napi_env env = nullptr;
    napi_value output = nullptr;
    napi_status status = napi_invalid_arg;
    std::string error;

    napi_value self = nullptr;
    void* native = nullptr;

private:
    napi_deferred deferred = nullptr;
    napi_async_work work = nullptr;
    napi_ref callbackRef = nullptr;
    napi_ref selfRef = nullptr;

    NapiAsyncExecute execute = nullptr;
    NapiAsyncComplete complete = nullptr;
    std::shared_ptr<ContextBase> hold; /* cross thread data */

    friend class NapiQueue;
};

/* check condition related to argc/argv, return and logging. */
#define CHECK_ARGS(ctxt, condition, message)                        \
    do {                                                               \
        if (!(condition)) {                                            \
            (ctxt)->status = napi_invalid_arg;                         \
            (ctxt)->error = std::string(message);                      \
            ZLOGE("test (" #condition ") failed: " message);           \
            return;                                                    \
        }                                                              \
    } while (0)

#define CHECK_STATUS(ctxt, message)                                 \
    do {                                                               \
        if ((ctxt)->status != napi_ok) {                               \
            (ctxt)->error = std::string(message);                      \
            ZLOGE("test (ctxt->status == napi_ok) failed: " message);  \
            return;                                                    \
        }                                                              \
    } while (0)

/* check condition, return and logging if condition not true. */
#define CHECK_RETURN(condition, message, retVal)             \
    do {                                                     \
        if (!(condition)) {                                  \
            ZLOGE("test (" #condition ") failed: " message); \
            return retVal;                                   \
        }                                                    \
    } while (0)

#define CHECK_RETURN_VOID(condition, message)                \
    do {                                                     \
        if (!(condition)) {                                  \
            ZLOGE("test (" #condition ") failed: " message); \
            return;                                          \
        }                                                    \
    } while (0)

class NapiQueue {
public:
    static napi_value AsyncWork(napi_env env, std::shared_ptr<ContextBase> ctxt, const std::string& name,
        NapiAsyncExecute execute = NapiAsyncExecute(), NapiAsyncComplete complete = NapiAsyncComplete());

private:
    enum {
        /* AsyncCallback / Promise output result index  */
        RESULT_ERROR = 0,
        RESULT_DATA = 1,
        RESULT_ALL = 2
    };
    static void GenerateOutput(ContextBase* ctxt);
};
}
#endif // OHOS_NAPI_QUEUE_H

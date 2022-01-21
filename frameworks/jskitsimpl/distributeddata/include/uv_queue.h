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
#ifndef OHOS_UV_QUEUE_H
#define OHOS_UV_QUEUE_H
#include <functional>

#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "uv.h"

namespace OHOS::DistributedData {
class UvQueue {
    using NapiArgsGenerator = std::function<void(napi_env env, int& argc, napi_value* argv)>;

public:
    UvQueue(napi_env env, napi_value callback);
    virtual ~UvQueue();

    bool operator==(napi_value value);

    void CallFunction(NapiArgsGenerator genArgs = NapiArgsGenerator());

private:
    napi_env env_ = nullptr;
    napi_ref callback_ = nullptr;
    NapiArgsGenerator args;
    uv_loop_s* loop_ = nullptr;
};
}
#endif // OHOS_UV_QUEUE_H

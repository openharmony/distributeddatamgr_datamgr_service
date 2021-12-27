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
#define LOG_TAG "UvQueue"
#include "uv_queue.h"
#include "log_print.h"
#include "napi_queue.h"

namespace OHOS::DistributedData {
UvQueue::UvQueue(napi_env env, napi_value callback)
    : env_(env)
{
    napi_create_reference(env, callback, 1, &callback_);
    napi_get_uv_event_loop(env, &loop_);
}

UvQueue::~UvQueue()
{
    ZLOGD("no memory leak for queue-callback");
    napi_delete_reference(env_, callback_);
}

bool UvQueue::operator==(napi_value value)
{
    napi_value callback = nullptr;
    napi_get_reference_value(env_, callback_, &callback);

    bool isEquals = false;
    napi_strict_equals(env_, value, callback, &isEquals);
    return isEquals;
}

void UvQueue::CallFunction(NapiArgsGenerator genArgs)
{
    uv_work_t* work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        ZLOGE("no memory for uv_work_t");
        return;
    }
    work->data = this;
    this->args = std::move(genArgs);

    uv_queue_work(
        loop_, work, [](uv_work_t* work) {},
        [](uv_work_t* work, int uvstatus) {
            auto queue = static_cast<UvQueue*>(work->data);
            int argc = 0;
            napi_value argv[ARGC_MAX] = { nullptr };
            if (queue->args) {
                queue->args(queue->env_, argc, argv);
            }
            ZLOGD("queue uv_after_work_cb");

            napi_value callback = nullptr;
            napi_get_reference_value(queue->env_, queue->callback_, &callback);
            napi_value global = nullptr;
            napi_get_global(queue->env_, &global);
            napi_value result;
            napi_status status = napi_call_function(queue->env_, global, callback, argc, argv, &result);
            if (status != napi_ok) {
                ZLOGE("notify data change failed status:%{public}d callback:%{public}p", status, callback);
            }
            delete work;
            work = nullptr;
        });
}
}

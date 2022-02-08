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
    work->data = new UvEntry{env_, callback_, std::move(genArgs)};
    uv_queue_work(
        loop_, work, [](uv_work_t* work) {},
        [](uv_work_t* work, int uvstatus) {
            auto *entry = static_cast<UvEntry*>(work->data);
            int argc = ARGC_MAX;
            napi_value argv[ARGC_MAX] = { nullptr };
            if (entry->args) {
                entry->args(entry->env, argc, argv);
            }
            ZLOGD("queue uv_after_work_cb");

            napi_value callback = nullptr;
            napi_get_reference_value(entry->env, entry->callback, &callback);
            napi_value global = nullptr;
            napi_get_global(entry->env, &global);
            napi_value result;
            napi_status status = napi_call_function(entry->env, global, callback, argc, argv, &result);
            if (status != napi_ok) {
                ZLOGE("notify data change failed status:%{public}d callback:%{public}p", status, callback);
            }
            delete entry;
            delete work;
            work = nullptr;
        });
}
}

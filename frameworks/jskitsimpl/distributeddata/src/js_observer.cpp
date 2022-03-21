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

#include "js_observer.h"

namespace OHOS::DistributedData {
void JSObserver::Clear()
{
    // Clear() run in js main thread, so it serial run with AsyncCall() lambda, so we can use no lock.
    if (callback_ == nullptr) {
        return ;
    }
    napi_delete_reference(uvQueue_->GetEnv(), callback_);
    callback_ = nullptr;
}

JSObserver::JSObserver(std::shared_ptr<UvQueue> uvQueue, napi_value callback)
    : uvQueue_(uvQueue)
{
    napi_create_reference(uvQueue_->GetEnv(), callback, 1, &callback_);
}

JSObserver::~JSObserver()
{
}

napi_ref JSObserver::GetCallback()
{
    return callback_;
}

void JSObserver::AsyncCall(UvQueue::NapiArgsGenerator genArgs)
{
    if (callback_ == nullptr) {
        return;
    }

    uvQueue_->AsyncCall([observer = shared_from_this()](napi_env env) -> napi_value {
        // the lambda run in js main thread, so it serial run with Clear(), so we can use no lock.
        if (observer->callback_ == nullptr) {
            return nullptr;
        }
        napi_value callback = nullptr;
        napi_get_reference_value(env, observer->callback_, &callback);
        return callback;
        }, genArgs);
}
} // namespace OHOS::DistributedData

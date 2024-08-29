/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cloud/cloud_lock_event.h"

namespace OHOS::DistributedData {
CloudLockEvent::CloudLockEvent(int32_t evtId, StoreInfo storeInfo, Callback callback)
    :CloudEvent(evtId, storeInfo), callback_(std::move(callback))
{
}
CloudLockEvent::Callback CloudLockEvent::GetCallback() const
{
    return callback_;
}
} // namespace OHOS::DistributedData
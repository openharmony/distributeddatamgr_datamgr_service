/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cloud/cloud_event.h"

namespace OHOS::DistributedData {
CloudEvent::CloudEvent(int32_t evtId, CloudEvent::StoreInfo storeInfo, const std::string &featureName)
    : Event(evtId), featureName_(featureName), storeInfo_(storeInfo)
{
}

const std::string& CloudEvent::GetFeatureName() const
{
    return featureName_;
}

const CloudEvent::StoreInfo& CloudEvent::GetStoreInfo() const
{
    return storeInfo_;
}
}
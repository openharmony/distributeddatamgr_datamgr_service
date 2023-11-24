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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARE_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARE_EVENT_H
#include "cloud/cloud_event.h"
#include "store/general_value.h"
#include "store/cursor.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT CloudShareEvent : public CloudEvent {
public:
    using Callback = std::function<void(int32_t, std::shared_ptr<DistributedData::Cursor>)>;
    CloudShareEvent(StoreInfo storeInfo, std::shared_ptr<GenQuery> query, Callback callback);
    ~CloudShareEvent() override = default;
    std::shared_ptr<GenQuery> GetQuery() const;
    Callback GetCallback() const;

private:
    std::shared_ptr<GenQuery> query_;
    Callback callback_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_SHARE_EVENT_H

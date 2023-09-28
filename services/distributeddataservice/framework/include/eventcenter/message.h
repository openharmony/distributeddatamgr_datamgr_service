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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_MESSAGE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_MESSAGE_H
#include <cstdint>
#include <functional>
#include <memory>

#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class Message {
public:
    enum MessageId : int32_t {
        MSG_INVALID,
        MSG_CLOUD = 0x1000
    };
    API_EXPORT Message(int32_t messageId);
    API_EXPORT virtual ~Message();
    API_EXPORT int32_t GetMessageId() const;

private:
    int32_t messageId_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_MESSAGE_H

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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_CHANGE_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_CHANGE_EVENT_H
#include <string>
#include "eventcenter/event.h"
namespace OHOS::DistributedData {
class MatrixEvent : public Event {
public:
    MatrixEvent(int32_t evtId, const std::string &device, uint16_t mask)
        : Event(evtId), mask_(mask), deviceId_(device)
    {
    }
    uint16_t GetMask() const
    {
        return mask_;
    }

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

private:
    uint16_t mask_;
    std::string deviceId_;
};
}
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_CHANGE_EVENT_H

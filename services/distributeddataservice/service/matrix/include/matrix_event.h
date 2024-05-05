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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_EVENT_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_EVENT_H
#include <string>

#include "eventcenter/event.h"
#include "utils/ref_count.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT MatrixEvent : public Event {
public:
    struct MatrixData {
        static constexpr uint16_t INVALID_LEVEL = 0xFFFF;
        static constexpr uint32_t INVALID_VALUE = 0xFFFFFFFF;
        static constexpr uint16_t INVALID_LENGTH = 0;
        uint16_t dynamic = INVALID_LEVEL;
        uint16_t statics = INVALID_LEVEL;
        uint32_t switches = INVALID_VALUE;
        uint16_t switchesLen = INVALID_LENGTH;
        bool IsValid() const;
    };
    MatrixEvent(int32_t evtId, const std::string &device, const MatrixData &data);
    ~MatrixEvent() = default;
    MatrixData GetMatrixData() const;
    std::string GetDeviceId() const;
    void SetRefCount(RefCount refCount);
    RefCount StealRefCount() const;
    bool Equals(const Event &event) const override;

private:
    MatrixData data_;
    std::string deviceId_;
    mutable RefCount refCount_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_MATRIX_MATRIX_EVENT_H

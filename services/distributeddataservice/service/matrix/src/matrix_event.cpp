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
#include "matrix_event.h"
namespace OHOS::DistributedData {
MatrixEvent::MatrixEvent(int32_t evtId, const std::string &device, const MatrixData &data)
    : Event(evtId), data_(data), deviceId_(device)
{
}

MatrixEvent::MatrixData MatrixEvent::GetMatrixData() const
{
    return data_;
}

std::string MatrixEvent::GetDeviceId() const
{
    return deviceId_;
}

bool MatrixEvent::Equals(const Event &event) const
{
    auto &evt = static_cast<const MatrixEvent &>(event);
    return deviceId_ == evt.deviceId_;
}

void MatrixEvent::SetRefCount(RefCount refCount)
{
    refCount_ = std::move(refCount);
}

RefCount MatrixEvent::StealRefCount() const
{
    return std::move(refCount_);
}

bool MatrixEvent::MatrixData::IsValid() const
{
    return !(dynamic == INVALID_LEVEL && statics == INVALID_LEVEL &&
        switches == INVALID_VALUE && switchesLen == INVALID_LENGTH);
}
} // namespace OHOS::DistributedData
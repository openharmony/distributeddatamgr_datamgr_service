/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_DEVICE_MATRIX_MOCK_H
#define OHOS_DEVICE_MATRIX_MOCK_H

#include <gmock/gmock.h>

#include "device_matrix.h"

namespace OHOS::DistributedData {
class BDeviceMatrix {
public:
    virtual std::pair<bool, uint16_t> GetMask(const std::string &, DeviceMatrix::LevelType) = 0;
    virtual std::pair<bool, uint16_t> GetRemoteMask(const std::string &, DeviceMatrix::LevelType) = 0;
    BDeviceMatrix() = default;
    virtual ~BDeviceMatrix() = default;
    static inline std::shared_ptr<BDeviceMatrix> deviceMatrix = nullptr;
};
class DeviceMatrixMock : public BDeviceMatrix {
public:
    MOCK_METHOD((std::pair<bool, uint16_t>), GetMask, (const std::string &, DeviceMatrix::LevelType), (override));
    MOCK_METHOD((std::pair<bool, uint16_t>), GetRemoteMask, (const std::string &, DeviceMatrix::LevelType), (override));
};
} // namespace OHOS::DistributedData
#endif //OHOS_DEVICE_MATRIX_MOCK_H

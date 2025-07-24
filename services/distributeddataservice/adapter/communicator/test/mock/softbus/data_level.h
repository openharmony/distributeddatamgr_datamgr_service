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

#ifndef MOCK_SOFTBUS_DATA_LEVEL_H
#define MOCK_SOFTBUS_DATA_LEVEL_H

#include <stdint.h>

#include "softbus_error_code.h"

struct DataLevel {
    uint16_t dynamicLevel;
    uint16_t staticLevel;
    uint32_t switchLevel;
    uint16_t switchLength;
};

struct IDataLevelCb {
    void (*onDataLevelChanged)(const char *networkId, const DataLevel dataLevel);
};

void ConfigReturnCode(SoftBusErrNo errorCode);

int32_t RegDataLevelChangeCb(const char *pkgName, IDataLevelCb *callback);

int32_t UnregDataLevelChangeCb(const char *pkgName);

int32_t SetDataLevel(const DataLevel *dataLevel);

#endif // MOCK_SOFTBUS_DATA_LEVEL_H
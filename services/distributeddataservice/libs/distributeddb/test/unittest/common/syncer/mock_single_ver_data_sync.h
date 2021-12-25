/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef MOCK_SINGLE_VER_DATA_SYNC_H
#define MOCK_SINGLE_VER_DATA_SYNC_H

#include <gmock/gmock.h>
#include "single_ver_data_sync.h"

namespace DistributedDB {
class MockSingleVerDataSync : public SingleVerDataSync {
public:
    MOCK_METHOD4(SendDataAck, int(SingleVerSyncTaskContext *, const Message *, int32_t, WaterMark));
};
} // namespace DistributedDB

#endif // MOCK_SINGLE_VER_DATA_SYNC_H

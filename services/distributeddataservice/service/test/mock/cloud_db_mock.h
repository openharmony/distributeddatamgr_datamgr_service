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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_CLOUD_DB_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_CLOUD_DB_MOCK_H

#include <gmock/gmock.h>

#include "cloud/cloud_db.h"

namespace OHOS::DistributedData {

class MockCloudDB : public CloudDB {
public:
    using Watcher = GeneralWatcher;
    using Async = std::function<void(std::map<std::string, std::map<std::string, int32_t>>)>;
    using Devices = std::vector<std::string>;

    MOCK_METHOD(int32_t, Execute, (const std::string &, const std::string &, const VBucket &), (override));
    MOCK_METHOD(int32_t, BatchInsert, (const std::string &, VBuckets &&, VBuckets &), (override));
    MOCK_METHOD(int32_t, BatchUpdate, (const std::string &, VBuckets &&, VBuckets &), (override));
    MOCK_METHOD(int32_t, BatchUpdate, (const std::string &, VBuckets &&, const VBuckets &), (override));
    MOCK_METHOD(int32_t, BatchDelete, (const std::string &, VBuckets &), (override));
    MOCK_METHOD(
        (std::pair<int32_t, std::shared_ptr<Cursor>>), Query, (const std::string &, const VBucket &), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), Query, (GenQuery &, const VBucket &), (override));
    MOCK_METHOD(int32_t, PreSharing, (const std::string &, VBuckets &), (override));
    MOCK_METHOD(int32_t, Sync, (const Devices &, int32_t, const GenQuery &, Async, int32_t), (override));
    MOCK_METHOD(int32_t, Watch, (int32_t, Watcher &), (override));
    MOCK_METHOD(int32_t, Unwatch, (int32_t, Watcher &), (override));
    MOCK_METHOD(int32_t, Lock, (), (override));
    MOCK_METHOD(int32_t, Heartbeat, (), (override));
    MOCK_METHOD(int32_t, Unlock, (), (override));
    MOCK_METHOD(int64_t, AliveTime, (), (override));
    MOCK_METHOD(int32_t, Close, (), (override));
    MOCK_METHOD((std::pair<int32_t, std::string>), GetEmptyCursor, (const std::string &), (override));
    MOCK_METHOD(void, SetPrepareTraceId, (const std::string &), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>),
        QueryAllGid, (const std::string &, const VBucket &), (override));
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTEDDATA_SERVICE_CLOUD_DB_MOCK_H

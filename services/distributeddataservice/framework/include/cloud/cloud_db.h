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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_H
#include <functional>
#include <string>

#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
#include "visibility.h"
namespace OHOS::DistributedData {
class API_EXPORT CloudDB {
public:
    using Watcher = GeneralWatcher;
    using Async = std::function<void(std::map<std::string, std::map<std::string, int32_t>>)>;
    using Devices = std::vector<std::string>;
    virtual ~CloudDB() = default;

    virtual int32_t Execute(const std::string &table, const std::string &sql, const VBucket &extend);

    virtual int32_t BatchInsert(const std::string &table, VBuckets &&values, VBuckets &extends);

    virtual int32_t BatchUpdate(const std::string &table, VBuckets &&values, VBuckets &extends);

    virtual int32_t BatchUpdate(const std::string &table, VBuckets &&values, const VBuckets &extends);

    virtual int32_t BatchDelete(const std::string &table, const VBuckets &extends);

    virtual std::shared_ptr<Cursor> Query(const std::string &table, const VBucket &extend);

    virtual std::shared_ptr<Cursor> Query(GenQuery &query, const VBucket &extend);

    virtual int32_t PreSharing(const std::string &table, VBuckets &extend);

    virtual int32_t Sync(const Devices &devices, int32_t mode, const GenQuery &query, Async async, int32_t wait);

    virtual int32_t Watch(int32_t origin, Watcher &watcher);

    virtual int32_t Unwatch(int32_t origin, Watcher &watcher);

    virtual int32_t Lock();

    virtual int32_t Heartbeat();

    virtual int32_t Unlock();

    virtual int64_t AliveTime();

    virtual int32_t Close();

    virtual std::pair<int32_t, std::string> GetEmptyCursor(const std::string &tableName);
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CLOUD_CLOUD_DB_H

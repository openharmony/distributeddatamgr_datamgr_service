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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_DB_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_DB_IMPL_H

#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "cloud_extension.h"
#include "store/cursor.h"
#include "store/general_value.h"

namespace OHOS::CloudData {
using DBVBucket = DistributedData::VBucket;
using DBVBuckets = DistributedData::VBuckets;
using DBCursor = DistributedData::Cursor;
using DBMeta = DistributedData::Database;
class CloudDbImpl : public DistributedData::CloudDB {
public:
    explicit CloudDbImpl(OhCloudExtCloudDatabase *database);
    ~CloudDbImpl();
    int32_t Execute(const std::string &table, const std::string &sql, const DBVBucket &extend) override;
    int32_t BatchInsert(const std::string &table, DBVBuckets &&values, DBVBuckets &extends) override;
    int32_t BatchUpdate(const std::string &table, DBVBuckets &&values, DBVBuckets &extends) override;
    int32_t BatchUpdate(const std::string &table, DBVBuckets &&values, const DBVBuckets &extends) override;
    int32_t BatchDelete(const std::string &table, const DBVBuckets &extends) override;
    std::shared_ptr<DBCursor> Query(const std::string &table, const DBVBucket &extend) override;
    int32_t Lock() override;
    int32_t Heartbeat() override;
    int32_t Unlock() override;
    int64_t AliveTime() override;
    int32_t Close() override;

private:

    OhCloudExtCloudDatabase *database_ = nullptr;
    int32_t interval_ = 0;
};
} // namespace OHOS::CloudData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_EXTENSION_CLOUD_DB_IMPL_H

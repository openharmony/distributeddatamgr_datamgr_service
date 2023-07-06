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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H
#include <functional>
#include <memory>

#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
namespace OHOS::DistributedData {
class CloudDB;
class AssetLoader;
struct Database;
class GeneralStore {
public:
    using Watcher = GeneralWatcher;
    using DetailAsync = GenAsync;
    using Devices = std::vector<std::string>;
    enum SyncMode {
        NEARBY_BEGIN,
        NEARBY_PUSH = NEARBY_BEGIN,
        NEARBY_PULL,
        NEARBY_PULL_PUSH,
        NEARBY_END,
        CLOUD_BEGIN = 4,
        CLOUD_TIME_FIRST = CLOUD_BEGIN,
        CLOUD_NATIVE_FIRST,
        CLOUD_ClOUD_FIRST,
        CLOUD_END,
        MODE_BUTT = CLOUD_END,
    };
    enum CleanMode {
        NEARBY_DATA = 0,
        CLOUD_DATA,
        CLOUD_INFO,
        LOCAL_DATA,
    };

    struct BindInfo {
        BindInfo(std::shared_ptr<CloudDB> db = nullptr, std::shared_ptr<AssetLoader> loader = nullptr)
            : db_(std::move(db)), loader_(std::move(loader))
        {
        }
        std::shared_ptr<CloudDB> db_;
        std::shared_ptr<AssetLoader> loader_;
    };
    virtual ~GeneralStore() = default;

    virtual int32_t Bind(const Database &database, BindInfo bindInfo) = 0;

    virtual bool IsBound() = 0;

    virtual int32_t Execute(const std::string &table, const std::string &sql) = 0;

    virtual int32_t BatchInsert(const std::string &table, VBuckets &&values) = 0;

    virtual int32_t BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values) = 0;

    virtual int32_t Delete(const std::string &table, const std::string &sql, Values &&args) = 0;

    virtual std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) = 0;

    virtual std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) = 0;

    virtual int32_t Sync(const Devices &devices, int32_t mode, GenQuery &query, DetailAsync async, int32_t wait) = 0;

    virtual int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) = 0;

    virtual int32_t Watch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t Unwatch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t Close() = 0;

    virtual int32_t AddRef() = 0;

    virtual int32_t Release()  = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H

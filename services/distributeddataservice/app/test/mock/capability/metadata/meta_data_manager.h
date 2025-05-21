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

#ifndef MOCK_META_DATA_MANAGER_H
#define MOCK_META_DATA_MANAGER_H

#include <functional>

#include "metadata/capability_meta_data.h"
#include "concurrent_map.h"
#include "executor_pool.h"

namespace OHOS::DistributedData {
class MetaDataManager {
public:
    enum Action : int32_t {
        INSERT = 0,
        UPDATE = 1,
        DELETE = 2,
    };

    using Observer = std::function<bool(const std::string &, const std::string &, int32_t)>;

    static MetaDataManager &GetInstance();
    void Init(std::shared_ptr<ExecutorPool> executors);
    bool SaveMeta(const std::string &key, const CapMetaData &value);
    bool LoadMeta(const std::string &key, CapMetaData &value);
    bool Subscribe(std::string prefix, Observer observer);
    void UnSubscribe();
    bool UpdateMeta(const std::string &key, const CapMetaData &value);
    bool DeleteMeta(const std::string &key, const CapMetaData &value);

private:
    MetaDataManager() = default;
    ~MetaDataManager() = default;

    Observer capObserver_;
    ConcurrentMap<std::string, CapMetaData> capabilities_ {};
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedData
#endif // MOCK_META_DATA_MANAGER_H
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

#ifndef UDMF_RUNTIMESTORE_H
#define UDMF_RUNTIMESTORE_H

#include "distributed_kv_data_manager.h"
#include "single_kvstore.h"

#include "store.h"

namespace OHOS {
namespace UDMF {
class RuntimeStore final : public Store {
public:
    explicit RuntimeStore(const std::string &storeId);
    ~RuntimeStore();
    Status Put(const UnifiedData &unifiedData) override;
    Status Get(const std::string &key, UnifiedData &unifiedData) override;
    Status GetSummary(const std::string &key, Summary &summary) override;
    Status Update(const UnifiedData &unifiedData) override;
    Status Delete(const std::string &key) override;
    Status DeleteBatch(const std::vector<std::string> &unifiedKeys) override;
    Status Sync(const std::vector<std::string> &devices) override;
    Status Clear() override;
    Status GetBatchData(const std::string &dataPrefix, std::vector<UnifiedData> &unifiedDataSet) override;
    void Close() override;
    bool Init() override;

private:
    static const DistributedKv::AppId APP_ID;
    static const std::string DATA_PREFIX;
    static const std::string BASE_DIR;
    static constexpr std::int32_t SLASH_COUNT_IN_KEY = 4;
    static constexpr std::int32_t MAX_BATCH_SIZE = 128;
    DistributedKv::DistributedKvDataManager dataManager_;
    std::shared_ptr<DistributedKv::SingleKvStore> kvStore_;
    DistributedKv::StoreId storeId_;
    Status GetEntries(const std::string &dataPrefix, std::vector<DistributedKv::Entry> &entries);
    Status PutEntries(const std::vector<DistributedKv::Entry> &entries);
    Status DeleteEntries(const std::vector<DistributedKv::Key> &keys);
    Status UnMarshalEntries(
        const std::string &key, std::vector<DistributedKv::Entry> &entries, UnifiedData &unifiedData);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_RUNTIMESTORE_H

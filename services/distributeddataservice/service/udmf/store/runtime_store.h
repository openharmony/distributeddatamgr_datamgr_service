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

#include "store.h"
#include "kv_store_delegate_manager.h"
#include "metadata/store_meta_data.h"

namespace OHOS {
namespace UDMF {
using DevNameMap = std::map<std::string, std::string>;
using DevSyncProcessMap = std::map<std::string, DistributedDB::DeviceSyncProcess>;
class RuntimeStore final : public Store {
public:
    explicit RuntimeStore(const std::string &storeId);
    ~RuntimeStore();
    Status Put(const UnifiedData &unifiedData) override;
    Status Get(const std::string &key, UnifiedData &unifiedData) override;
    Status PutSummary(const std::string &key, const Summary &summary) override;
    Status GetSummary(const std::string &key, Summary &summary) override;
    Status Update(const UnifiedData &unifiedData) override;
    Status Delete(const std::string &key) override;
    Status DeleteBatch(const std::vector<std::string> &unifiedKeys) override;
    Status Sync(const std::vector<std::string> &devices) override;
    Status Sync(const std::vector<std::string> &devices, ProcessCallback callback) override;
    Status Clear() override;
    Status GetBatchData(const std::string &dataPrefix, std::vector<UnifiedData> &unifiedDataSet) override;
    Status PutLocal(const std::string &key, const std::string &value) override;
    Status GetLocal(const std::string &key, std::string &value) override;
    Status DeleteLocal(const std::string &key) override;
    Status PutRuntime(const std::string &key, const Runtime &runtime) override;
    Status GetRuntime(const std::string &key, Runtime &runtime) override;
    void Close() override;
    bool Init() override;

private:
    static constexpr const char *DATA_PREFIX = "udmf://";
    static constexpr std::int32_t SLASH_COUNT_IN_KEY = 4;
    static constexpr std::int32_t MAX_BATCH_SIZE = 128;
    std::shared_ptr<DistributedDB::KvStoreDelegateManager> delegateManager_ = nullptr;
    std::shared_ptr<DistributedDB::KvStoreNbDelegate> kvStore_;
    std::string storeId_;
    void SetDelegateManager(const std::string &dataDir, const std::string &appId, const std::string &userId,
        const std::string &subUser);
    bool SaveMetaData();
    Status GetEntries(const std::string &dataPrefix, std::vector<DistributedDB::Entry> &entries);
    Status PutEntries(const std::vector<DistributedDB::Entry> &entries);
    Status DeleteEntries(const std::vector<DistributedDB::Key> &keys);
    Status UnmarshalEntries(
        const std::string &key, std::vector<DistributedDB::Entry> &entries, UnifiedData &unifiedData);
    bool BuildMetaDataParam(DistributedData::StoreMetaData &metaData);
    void NotifySyncProcss(const DevSyncProcessMap &processMap, ProcessCallback callback,
        const DevNameMap &deviceNameMap);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_RUNTIMESTORE_H

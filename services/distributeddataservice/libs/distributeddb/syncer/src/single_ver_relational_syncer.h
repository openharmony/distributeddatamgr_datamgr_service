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
#ifndef RELATIONAL_SYNCER_H
#define RELATIONAL_SYNCER_H
#ifdef RELATIONAL_STORE
#include "single_ver_syncer.h"
namespace DistributedDB {
class SingleVerRelationalSyncer final : public SingleVerSyncer {
public:
    SingleVerRelationalSyncer() = default;
    ~SingleVerRelationalSyncer() override = default;

    int Initialize(ISyncInterface *syncInterface, bool isNeedActive) override;

    // Sync function. use SyncParma to reduce parameter.
    int Sync(const SyncParma &param, uint64_t connectionId) override;

    void EnableAutoSync(bool enable) override;

    void LocalDataChanged(int notifyEvent) override;

protected:

    int PrepareSync(const SyncParma &param, uint32_t syncId, uint64_t connectionId) override;

    int SyncConditionCheck(QuerySyncObject &query, int mode, bool isQuerySync,
        const std::vector<std::string> &devices) const override;

private:

    int GenerateEachSyncTask(const SyncParma &param, uint32_t syncId,
        const std::vector<QuerySyncObject> &tablesQuery, uint64_t connectionId, std::set<uint32_t> &subSyncIdSet);

    void DoRollBack(std::set<uint32_t> &subSyncIdSet);

    void DoOnComplete(const SyncParma &param, uint32_t syncId);
    void DoOnSubSyncComplete(const uint32_t subSyncId, const uint32_t syncId,
        const SyncParma &param, const std::map<std::string, int> &devicesMap);

    void SchemaChangeCallback();

    mutable std::mutex syncMapLock_;
    std::map<uint32_t, std::set<uint32_t>> fullSyncIdMap_;
    std::map<uint32_t, std::map<std::string, std::map<std::string, int>>> resMap_;
};
}
#endif
#endif // RELATIONAL_SYNCER_H
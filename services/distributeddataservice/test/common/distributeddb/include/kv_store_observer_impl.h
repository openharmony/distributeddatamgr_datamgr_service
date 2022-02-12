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
#ifndef KVSTORE_OBSERVER_IMPL_H
#define KVSTORE_OBSERVER_IMPL_H

#include <condition_variable>
#include <list>
#include <mutex>
#include "distributeddb_data_generator.h"
#include "kv_store_changed_data.h"
#include "kv_store_observer.h"

enum ListType {
    INSERT_LIST = 0,
    UPDATE_LIST = 1,
    DELETE_LIST = 2
};

class KvStoreObserverImpl final : public DistributedDB::KvStoreObserver {
public:
    void OnChange(const DistributedDB::KvStoreChangedData &data);

    KvStoreObserverImpl();

    ~KvStoreObserverImpl();

    KvStoreObserverImpl(bool isCumulatedFlag) : isSaveCumulatedData_(isCumulatedFlag)
    {
    }
    KvStoreObserverImpl(const KvStoreObserverImpl &);
    KvStoreObserverImpl& operator=(const KvStoreObserverImpl &);

    const std::list<DistributedDB::Entry> GetInsertList() const;

    const std::list<DistributedDB::Entry> GetUpdateList() const;

    const std::list<DistributedDB::Entry> GetDeleteList() const;

    int GetChanged() const;

    void WaitUntilReachChangeCount(unsigned int countGoal, uint32_t timeout = 0) const; // timeout in second
    // timeout in second
    void WaitUntilReachRecordCount(unsigned int countExpect, ListType waitWhat, uint32_t timeout = 0) const;

    microClock_type GetOnChangeTime();

    void Clear();

    void SetCumulatedFlag(bool isSaveCumulatedData);

    bool GetCumulatedFlag() const;

    const std::list<DistributedDB::Entry> GetCumulatedInsertList() const;

    const std::list<DistributedDB::Entry> GetCumulatedUpdateList() const;

    const std::list<DistributedDB::Entry> GetCumulatedDeleteList() const;

private:
    std::list<DistributedDB::Entry> insertedEntries_ = {};
    std::list<DistributedDB::Entry> updatedEntries_ = {};
    std::list<DistributedDB::Entry> deleteEntries_ = {};
    unsigned int changed_ = 0;
    microClock_type onChangeTime_
        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
    bool isSaveCumulatedData_ = false;
    std::list<DistributedDB::Entry> cumulatedInsertList_ = {};
    std::list<DistributedDB::Entry> cumulatedUpdateList_ = {};
    std::list<DistributedDB::Entry> cumulatedDeleteList_ = {};
    // For waiting method
    mutable std::mutex waitChangeMutex_;
    mutable std::condition_variable waitChangeCv_;
};
#endif // KVSTORE_OBSERVER_IMPL_H
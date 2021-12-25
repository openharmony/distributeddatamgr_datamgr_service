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
#include "kv_store_observer_impl.h"

void KvStoreObserverImpl::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    onChangeTime_ = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
    insertedEntries_ = data.GetEntriesInserted();
    updatedEntries_ = data.GetEntriesUpdated();
    deleteEntries_ = data.GetEntriesDeleted();
    MST_LOG("insertedEntries_.size():%zu, updatedEntries_.size():%zu, deleteEntries_.size():%zu",
        insertedEntries_.size(), updatedEntries_.size(), deleteEntries_.size());
    if (isSaveCumulatedData_) {
        if (insertedEntries_.size() != 0) {
            for (const auto &entry : insertedEntries_) {
                cumulatedInsertList_.push_back(entry);
            }
        }
        if (updatedEntries_.size() != 0) {
            for (const auto &entry : updatedEntries_) {
                cumulatedUpdateList_.push_back(entry);
            }
        }
        if (deleteEntries_.size() != 0) {
            for (const auto &entry : deleteEntries_) {
                cumulatedDeleteList_.push_back(entry);
            }
        }
    }
    {
        std::unique_lock<std::mutex> waitChangeLock(waitChangeMutex_);
        changed_++;
        MST_LOG("comes a change,changed[%d]!!!", changed_);
    }
    waitChangeCv_.notify_all();
}

KvStoreObserverImpl::KvStoreObserverImpl()
{
    onChangeTime_ = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
    isSaveCumulatedData_ = false;
    Clear();
}

KvStoreObserverImpl::~KvStoreObserverImpl()
{
}

KvStoreObserverImpl::KvStoreObserverImpl(const KvStoreObserverImpl &other)
{
    insertedEntries_ = other.insertedEntries_;
    updatedEntries_ = other.updatedEntries_;
    deleteEntries_ = other.deleteEntries_;
    changed_ = other.changed_;
    onChangeTime_ = other.onChangeTime_;
    isSaveCumulatedData_ = other.isSaveCumulatedData_;
    cumulatedInsertList_ = other.cumulatedInsertList_;
    cumulatedUpdateList_ = other.cumulatedUpdateList_;
    cumulatedDeleteList_ = other.cumulatedDeleteList_;
}

KvStoreObserverImpl& KvStoreObserverImpl::operator=(const KvStoreObserverImpl &other)
{
    if (&other != this) {
        insertedEntries_ = other.insertedEntries_;
        updatedEntries_ = other.updatedEntries_;
        deleteEntries_ = other.deleteEntries_;
        changed_ = other.changed_;
        onChangeTime_ = other.onChangeTime_;
        isSaveCumulatedData_ = other.isSaveCumulatedData_;
        cumulatedInsertList_ = other.cumulatedInsertList_;
        cumulatedUpdateList_ = other.cumulatedUpdateList_;
        cumulatedDeleteList_ = other.cumulatedDeleteList_;
    }
    return *this;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetInsertList() const
{
    return insertedEntries_;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetUpdateList() const
{
    return updatedEntries_;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetDeleteList() const
{
    return deleteEntries_;
}

int KvStoreObserverImpl::GetChanged() const
{
    return changed_;
}

void KvStoreObserverImpl::WaitUntilReachChangeCount(unsigned int countGoal, uint32_t timeout) const // timeout in second
{
    if (countGoal == 0 || changed_ >= countGoal) {
        return;
    }
    // Change count has not reach countGoal
    auto waitChangeFunc = [this, countGoal]()->bool {
        MST_LOG("############################ realChanged=%d, countGoal=%d", this->changed_, countGoal);
        return this->changed_ >= countGoal;
    };
    MST_LOG("############################ BEGIN ############################");
    std::unique_lock<std::mutex> waitChangeLock(waitChangeMutex_);
    if (timeout == 0) {
        waitChangeCv_.wait(waitChangeLock, waitChangeFunc);
    } else {
        waitChangeCv_.wait_for(waitChangeLock, std::chrono::seconds(timeout), waitChangeFunc);
    }
    MST_LOG("############################ E N D ############################");
}

void KvStoreObserverImpl::WaitUntilReachRecordCount(unsigned int countExpect, ListType waitWhat, uint32_t timeout) const
{
    MST_LOG("###########################- BEGIN list type: %d -###########################", waitWhat);
    std::function<bool(void)> waitRecordsFunc;
    switch (waitWhat) {
        case INSERT_LIST:
            if (countExpect == 0 || cumulatedInsertList_.size() >= countExpect) {
                return;
            }
            // Change count has not reach countExpect
            waitRecordsFunc = [this, countExpect]()->bool {
                MST_LOG("[NbObserver][Wait] #### this->cumulatedInsertList_.size()=%zu, countExpect=%d ####",
                    this->cumulatedInsertList_.size(), countExpect);
                return this->cumulatedInsertList_.size() >= countExpect;
            };
            break;
        case UPDATE_LIST:
            if (countExpect == 0 || cumulatedUpdateList_.size() >= countExpect) {
                return;
            }
            // Change count has not reach countExpect
            waitRecordsFunc = [this, countExpect]()->bool {
                MST_LOG("[NbObserver][Wait] #### this->cumulatedUpdateList_.size()=%zu, countExpect=%d ####",
                    this->cumulatedUpdateList_.size(), countExpect);
                return this->cumulatedUpdateList_.size() >= countExpect;
            };
            break;
        case DELETE_LIST:
            if (countExpect == 0 || cumulatedDeleteList_.size() >= countExpect) {
                return;
            }
            // Change count has not reach countExpect
            waitRecordsFunc = [this, countExpect]()->bool {
                MST_LOG("[NbObserver][Wait] #### this->cumulatedDeleteList_.size()=%zu, countExpect=%d ####",
                    this->cumulatedDeleteList_.size(), countExpect);
                return this->cumulatedDeleteList_.size() >= countExpect;
            };
            break;
        default:
            break;
    }

    std::unique_lock<std::mutex> waitRecordsLock(waitChangeMutex_);
    if (timeout == 0) {
        waitChangeCv_.wait(waitRecordsLock, waitRecordsFunc);
    } else {
        waitChangeCv_.wait_for(waitRecordsLock, std::chrono::seconds(timeout), waitRecordsFunc);
    }

    MST_LOG("############################ E-N-D ############################");
}

microClock_type KvStoreObserverImpl::GetOnChangeTime()
{
    return onChangeTime_;
}

void KvStoreObserverImpl::Clear()
{
    insertedEntries_.clear();
    updatedEntries_.clear();
    deleteEntries_.clear();
    changed_ = 0;
    cumulatedInsertList_.clear();
    cumulatedUpdateList_.clear();
    cumulatedDeleteList_.clear();
}

void KvStoreObserverImpl::SetCumulatedFlag(bool isSaveCumulatedData)
{
    isSaveCumulatedData_ = isSaveCumulatedData;
}

bool KvStoreObserverImpl::GetCumulatedFlag() const
{
    return isSaveCumulatedData_;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetCumulatedInsertList() const
{
    return cumulatedInsertList_;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetCumulatedUpdateList() const
{
    return cumulatedUpdateList_;
}

const std::list<DistributedDB::Entry> KvStoreObserverImpl::GetCumulatedDeleteList() const
{
    return cumulatedDeleteList_;
}
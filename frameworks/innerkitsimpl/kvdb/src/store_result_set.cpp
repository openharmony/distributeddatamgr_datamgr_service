/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "StoreResultSet"
#include "store_result_set.h"

#include "dds_trace.h"
#include "log_print.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
StoreResultSet::StoreResultSet(DBResultSet *impl, std::shared_ptr<DBStore> dbStore)
    : impl_(impl), dbStore_(std::move(dbStore))
{
}

StoreResultSet::~StoreResultSet()
{
    if (impl_ != nullptr && dbStore_ != nullptr) {
        dbStore_->CloseResultSet(impl_);
        impl_ = nullptr;
    }
}

int StoreResultSet::GetCount() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->GetCount();
}

int StoreResultSet::GetPosition() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->GetPosition();
}

bool StoreResultSet::MoveToFirst()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->MoveToFirst();
}
bool StoreResultSet::MoveToLast()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->MoveToLast();
}

bool StoreResultSet::MoveToNext()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->MoveToNext();
}

bool StoreResultSet::MoveToPrevious()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->MoveToPrevious();
}

bool StoreResultSet::Move(int offset)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->Move(offset);
}

bool StoreResultSet::MoveToPosition(int position)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->MoveToPosition(position);
}

bool StoreResultSet::IsFirst() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->IsFirst();
}
bool StoreResultSet::IsLast() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->IsLast();
}

bool StoreResultSet::IsBeforeFirst() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->IsBeforeFirst();
}

bool StoreResultSet::IsAfterLast() const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    return impl_->IsAfterLast();
}

Status StoreResultSet::GetEntry(Entry &entry) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        return ALREADY_CLOSED;
    }
    DistributedDB::Entry dbEntry;
    auto dbStatus = impl_->GetEntry(dbEntry);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("failed! status:%{public}d, position:%{public}d", status, impl_->GetPosition());
        return status;
    }
    entry.key = ConvertKey(std::move(dbEntry.key));
    entry.value = std::move(dbEntry.value);
    return SUCCESS;
}

Status StoreResultSet::Close()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr || dbStore_ == nullptr) {
        return SUCCESS;
    }
    auto dbStatus = dbStore_->CloseResultSet(impl_);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == SUCCESS) {
        impl_ = nullptr;
        dbStore_ = nullptr;
    }
    return status;
}

Key StoreResultSet::ConvertKey(DistributedDB::Key &&key) const
{
    return Key(std::move(key));
}
} // namespace OHOS::DistributedKv
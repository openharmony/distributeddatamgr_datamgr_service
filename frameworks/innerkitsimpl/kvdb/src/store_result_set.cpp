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

#include "log_print.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
StoreResultSet::StoreResultSet(DBResultSet *impl, std::shared_ptr<DBStore> dbStore, const Convertor &convert)
    : impl_(impl), dbStore_(std::move(dbStore)), convert_(convert)
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
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return INVALID_COUNT;
    }

    return impl_->GetCount();
}

int StoreResultSet::GetPosition() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return INVALID_POSITION;
    }

    return impl_->GetPosition();
}

bool StoreResultSet::MoveToFirst()
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->MoveToFirst();
}
bool StoreResultSet::MoveToLast()
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->MoveToLast();
}

bool StoreResultSet::MoveToNext()
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->MoveToNext();
}

bool StoreResultSet::MoveToPrevious()
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->MoveToPrevious();
}

bool StoreResultSet::Move(int offset)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->Move(offset);
}

bool StoreResultSet::MoveToPosition(int position)
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->MoveToPosition(position);
}

bool StoreResultSet::IsFirst() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->IsFirst();
}
bool StoreResultSet::IsLast() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->IsLast();
}

bool StoreResultSet::IsBeforeFirst() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->IsBeforeFirst();
}

bool StoreResultSet::IsAfterLast() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return false;
    }

    return impl_->IsAfterLast();
}

Status StoreResultSet::GetEntry(Entry &entry) const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    if (impl_ == nullptr) {
        ZLOGW("already closed");
        return ALREADY_CLOSED;
    }

    DBEntry dbEntry;
    auto dbStatus = impl_->GetEntry(dbEntry);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("failed! status:%{public}d, position:%{public}d", status, impl_->GetPosition());
        return status;
    }
    std::string deviceId;
    entry.key = convert_.ToKey(std::move(dbEntry.key), deviceId);
    entry.value = std::move(dbEntry.value);
    return SUCCESS;
}

Status StoreResultSet::Close()
{
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
} // namespace OHOS::DistributedKv
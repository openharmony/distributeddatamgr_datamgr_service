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

#define LOG_TAG "RdbDeviceSyncer"

#include "rdb_device_syncer.h"
#include "log_print.h"
#include "rdb_syncer_factory.h"
#include "relational_store_manager.h"
#include "relational_store_delegate.h"

namespace OHOS::DistributedRdb {
RdbDeviceSyncer::RdbDeviceSyncer(const RdbSyncerParam &param, pid_t uid)
    : RdbSyncerImpl(param, uid), isInit_(false), manager_(nullptr), delegate_(nullptr)
{
    ZLOGI("construct %{public}s", param.storeName_.c_str());
}

RdbDeviceSyncer::~RdbDeviceSyncer()
{
    ZLOGI("destroy");
    if (manager_ != nullptr & delegate_ != nullptr) {
        manager_->CloseStore(delegate_);
    }
    delete manager_;
}

int RdbDeviceSyncer::Init()
{
    ZLOGI("enter");
    if (isInit_) {
        return 0;
    }
    manager_ = new(std::nothrow) DistributedDB::RelationalStoreManager(GetAppId(), GetUserId());
    if (manager_ == nullptr) {
        ZLOGE("malloc manager failed");
        return -1;
    }
    if (CreateMetaData() != 0) {
        ZLOGE("create meta data failed");
        return -1;
    }
    isInit_ = true;
    ZLOGI("success");
    return 0;
}

int RdbDeviceSyncer::CreateMetaData()
{
    ZLOGI("enter");
    return 0;
}

int RdbDeviceSyncer::SetDistributedTables(const std::vector<std::string> &tables)
{
    auto delegate = GetDelegate();
    if (delegate == nullptr) {
        return -1;
    }
    
    for (const auto& table : tables) {
        ZLOGI("%{public}s", table.c_str());
        if (delegate->CreateDistributedTable(table) != DistributedDB::DBStatus::OK) {
            ZLOGE("create distributed table failed");
            return -1;
        }
    }
    ZLOGE("create distributed table success");
    return 0;
}

DistributedDB::RelationalStoreDelegate* RdbDeviceSyncer::GetDelegate()
{
    if (manager_ == nullptr) {
        ZLOGE("manager_ is nullptr");
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (delegate_ == nullptr) {
        ZLOGI("create delegate");
        DistributedDB::RelationalStoreDelegate::Option option;
        DistributedDB::DBStatus status = manager_->OpenStore(GetPath(), GetStoreId(), option, delegate_);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("open store failed status=%{public}d", status);
            return nullptr;
        }
        ZLOGI("open store success");
    }
    return delegate_;
}
}


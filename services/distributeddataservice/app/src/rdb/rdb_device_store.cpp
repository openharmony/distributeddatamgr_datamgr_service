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

#define LOG_TAG "RdbDeviceStore"

#include "rdb_device_store.h"
#include "log_print.h"
#include "rdb_store_factory.h"
#include "relational_store_manager.h"
#include "relational_store_delegate.h"

namespace OHOS::DistributedKv {
void RdbDeviceStore::Initialize()
{
    RdbStoreFactory::Creator creator = RdbDeviceStore::CreateStore;
    RdbStoreFactory::RegisterCreator(RDB_DEVICE_COLLABORATION, creator);
}

RdbStore* RdbDeviceStore::CreateStore(const RdbStoreParam &param)
{
    ZLOGI("create device collaboration store");
    return new(std::nothrow) RdbDeviceStore(param);
}

RdbDeviceStore::RdbDeviceStore(const RdbStoreParam &param)
    : RdbStore(param), manager_(nullptr), delegate_(nullptr)
{
    ZLOGI("construct %{public}s", param.storeName_.c_str());
}

RdbDeviceStore::~RdbDeviceStore()
{
    ZLOGI("destroy");
    if (manager_ != nullptr & delegate_ != nullptr) {
        manager_->CloseStore(delegate_);
    }
    delete manager_;
}

int RdbDeviceStore::Init()
{
    ZLOGI("enter");
    manager_ = new(std::nothrow) DistributedDB::RelationalStoreManager(GetAppId(), GetUserId());
    if (manager_ == nullptr) {
        ZLOGE("malloc manager failed");
        return -1;
    }
    if (CreateMetaData() != 0) {
        ZLOGE("create meta data failed");
        return -1;
    }
    ZLOGI("success");
    return 0;
}

int RdbDeviceStore::CreateMetaData()
{
    ZLOGI("enter");
    return 0;
}

int RdbDeviceStore::SetDistributedTables(const std::vector<std::string> &tables)
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

DistributedDB::RelationalStoreDelegate* RdbDeviceStore::GetDelegate()
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


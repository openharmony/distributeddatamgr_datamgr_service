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

#define LOG_TAG "RdbStore"

#include "rdb_store.h"
#include "kvstore_utils.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
static std::string GetCurrentUserId()
{
    return "0";
}

RdbStore::RdbStore(const RdbStoreParam &param)
    : type_(param.type_), bundleName_(param.bundleName_), path_(param.path_),
      storeId_(param.storeName_)
{
    ZLOGI("construct %{public}s %{public}s %{public}s %{public}d",
          bundleName_.c_str(), userId_.c_str(), storeId_.c_str(), type_);
    appId_ = KvStoreUtils::GetAppIdByBundleName(bundleName_);
    userId_ = GetCurrentUserId();
    identifier_ = std::to_string(type_) + "-" + appId_ + "-" + userId_ + "-" + storeId_;
}

RdbStore::~RdbStore()
{
    ZLOGI("destroy %{public}s", storeId_.c_str());
}

bool RdbStore::operator==(const RdbStore& rhs) const
{
    return identifier_ == rhs.identifier_;
}

std::string RdbStore::GetBundleName() const
{
    return bundleName_;
}

std::string RdbStore::GetAppId() const
{
    return appId_;
}

std::string RdbStore::GetUserId() const
{
    return userId_;
}

std::string RdbStore::GetStoreId() const
{
    return storeId_;
}

std::string RdbStore::GetIdentifier() const
{
    return identifier_;
}

std::string RdbStore::GetPath() const
{
    return path_;
}
}
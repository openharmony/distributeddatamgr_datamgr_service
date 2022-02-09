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

#define LOG_TAG "RdbSyncerImpl"

#include "rdb_syncer_impl.h"
#include "account_delegate.h"
#include "checker/checker_manager.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
RdbSyncerImpl::RdbSyncerImpl(const RdbSyncerParam &param, pid_t uid)
    : type_(param.type_), bundleName_(param.bundleName_), path_(param.path_),
      storeId_(param.storeName_)
{
    ZLOGI("construct %{public}s %{public}s %{public}s %{public}d",
          bundleName_.c_str(), userId_.c_str(), storeId_.c_str(), type_);
    appId_ = CheckerManager::GetInstance().GetAppId(param.bundleName_, uid);
    userId_ = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    identifier_ = std::to_string(type_) + "-" + appId_ + "-" + userId_ + "-" + storeId_;
}

RdbSyncerImpl::~RdbSyncerImpl()
{
    ZLOGI("destroy %{public}s", storeId_.c_str());
}

bool RdbSyncerImpl::operator==(const RdbSyncerImpl& rhs) const
{
    return identifier_ == rhs.identifier_;
}

std::string RdbSyncerImpl::GetBundleName() const
{
    return bundleName_;
}

std::string RdbSyncerImpl::GetAppId() const
{
    return appId_;
}

std::string RdbSyncerImpl::GetUserId() const
{
    return userId_;
}

std::string RdbSyncerImpl::GetStoreId() const
{
    return storeId_;
}

std::string RdbSyncerImpl::GetIdentifier() const
{
    return identifier_;
}

std::string RdbSyncerImpl::GetPath() const
{
    return path_;
}
}
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

#define LOG_TAG "RdbWatcher"

#include "rdb_watcher.h"

#include "error/general_error.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedRdb {
using namespace DistributedData;
using Error = DistributedData::GeneralError;
RdbWatcher::RdbWatcher()
{
}

int32_t RdbWatcher::OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values)
{
    auto notifier = GetNotifier();
    if (notifier == nullptr) {
        return E_NOT_INIT;
    }
    DistributedRdb::Origin rdbOrigin;
    rdbOrigin.origin = origin.origin;
    rdbOrigin.dataType = origin.dataType;
    rdbOrigin.id = origin.id;
    rdbOrigin.store = origin.store;
    ZLOGD("store:%{public}s data change from :%{public}s, dataType:%{public}d, origin:%{public}d.",
        Anonymous::Change(rdbOrigin.store).c_str(),
        rdbOrigin.id.empty() ? "empty" : Anonymous::Change(*rdbOrigin.id.begin()).c_str(),
        rdbOrigin.dataType, rdbOrigin.origin);
    notifier->OnChange(rdbOrigin, primaryFields, std::move(values));
    return E_OK;
}

int32_t RdbWatcher::OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas)
{
    return E_OK;
}

sptr<RdbNotifierProxy> RdbWatcher::GetNotifier() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    return notifier_;
}

void RdbWatcher::SetNotifier(sptr<RdbNotifierProxy> notifier)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    if (notifier_ == notifier) {
        return;
    }
    notifier_ = notifier;
}
} // namespace OHOS::DistributedRdb

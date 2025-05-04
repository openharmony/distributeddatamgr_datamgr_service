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
#define LOG_TAG "LifeCyclePolicy"

#include "lifecycle_policy.h"

#include "log_print.h"
#include "preprocess/preprocess_utils.h"

namespace OHOS {
namespace UDMF {
using namespace std::chrono;

Status LifeCyclePolicy::OnGot(const UnifiedKey &key)
{
    auto store = StoreCache::GetInstance().GetStore(key.intention);
    if (store == nullptr) {
        ZLOGE("Get store failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    if (store->Delete(key.key) != E_OK) {
        ZLOGE("Remove data failed:%{public}s", key.intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

Status LifeCyclePolicy::OnStart(const std::string &intention)
{
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Store get failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    if (store->Clear() != E_OK) {
        ZLOGE("Data removal failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

Status LifeCyclePolicy::OnTimeout(const std::string &intention)
{
    auto store = StoreCache::GetInstance().GetStore(intention);
    if (store == nullptr) {
        ZLOGE("Store get failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    std::vector<std::string> timeoutKeys;
    Duration interval = INTERVAL;
    CheckFileMangerIntention(intention, interval);
    auto status = GetTimeoutKeys(store, INTERVAL, timeoutKeys);
    if (status != E_OK) {
        ZLOGE("Timeout keys get failed");
        return E_DB_ERROR;
    }
    if (store->DeleteBatch(timeoutKeys) != E_OK) {
        ZLOGE("Data removal failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

Status LifeCyclePolicy::GetTimeoutKeys(
    const std::shared_ptr<Store> &store, Duration interval, std::vector<std::string> &timeoutKeys)
{
    std::vector<UnifiedData> datas;
    auto status = store->GetBatchData(DATA_PREFIX, datas);
    if (status != E_OK) {
        ZLOGE("Get data failed.");
        return E_DB_ERROR;
    }
    if (datas.empty()) {
        ZLOGD("entries is empty.");
        return E_OK;
    }
    auto curTime = PreProcessUtils::GetTimestamp();
    for (const auto &data : datas) {
        if (data.GetRuntime() == nullptr) {
            ZLOGD("Runtime is null");
            return E_DB_ERROR;
        }
        if (curTime > data.GetRuntime()->createTime + duration_cast<milliseconds>(interval).count()
            || curTime < data.GetRuntime()->createTime) {
            timeoutKeys.push_back(data.GetRuntime()->key.key);
        }
    }
    return E_OK;
}

Status LifeCyclePolicy::CheckFileMangerIntention(const std::string &intention, Duration &interval)
{
    if (intention == UD_INTENTION_MAP.at(UD_INTENTION_SYSTEM_SHARE) ||
        intention == UD_INTENTION_MAP.at(UD_INTENTION_PICKER) ||
        intention == UD_INTENTION_MAP.at(UD_INTENTION_MENU)) {
        interval = SYSTEM_SHARE_INTERVAL;
    }
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
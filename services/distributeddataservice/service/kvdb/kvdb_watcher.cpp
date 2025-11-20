/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "KVDBWatcher"

#include "kvdb_watcher.h"

#include "error/general_error.h"
#include "kvdb_observer_proxy.h"
#include "log_print.h"
#include "types.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedKv {
using namespace DistributedData;
using Error = DistributedData::GeneralError;
KVDBWatcher::KVDBWatcher() {}

int32_t KVDBWatcher::OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values)
{
    auto store = origin.store;
    auto changeData = values.find(store);
    if (changeData != values.end()) {
        auto observer = GetObserver();
        if (observer == nullptr) {
            return E_NOT_INIT;
        }
        std::vector<std::string> keys[OP_BUTT]{};
        keys[OP_INSERT] = ConvertToKeys(changeData->second[OP_INSERT]);
        keys[OP_UPDATE] = ConvertToKeys(changeData->second[OP_UPDATE]);
        keys[OP_DELETE] = ConvertToKeys(changeData->second[OP_DELETE]);
        DataOrigin dataOrigin;
        dataOrigin.id = origin.id;
        dataOrigin.store = origin.store;
        observer->OnChange(dataOrigin, std::move(keys));
    }
    return E_OK;
}

int32_t KVDBWatcher::OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas)
{
    auto store = origin.store;
    auto changeData = datas.find(store);
    if (changeData != datas.end()) {
        auto observer = GetObserver();
        if (observer == nullptr) {
            return E_NOT_INIT;
        }
        auto inserts = ConvertToEntries(changeData->second[OP_INSERT]);
        auto updates = ConvertToEntries(changeData->second[OP_UPDATE]);
        auto deletes = ConvertToEntries(changeData->second[OP_DELETE]);
        ChangeNotification change(std::move(inserts), std::move(updates), std::move(deletes), {}, false);
        observer->OnChange(change);
    }
    return E_OK;
}
int32_t KVDBWatcher::OnChange(const std::string &storeId, const int32_t triggerMode)
{
    return E_OK;
}
sptr<IKvStoreObserver> KVDBWatcher::GetObserver() const
{
    std::shared_lock<decltype(mutex_)> lock(mutex_);
    return observer_;
}

void KVDBWatcher::SetObserver(sptr<IKvStoreObserver> observer)
{
    std::unique_lock<decltype(mutex_)> lock(mutex_);
    observer_ = std::move(observer);
}

std::vector<Entry> KVDBWatcher::ConvertToEntries(const std::vector<Values> &values)
{
    std::vector<Entry> changeData{};
    for (auto &info : values) {
        auto key = std::get_if<Bytes>(&info[0]);
        auto value = std::get_if<Bytes>(&info[1]);
        if (key == nullptr || value == nullptr) {
            continue;
        }
        Entry tmpEntry{ *key, *value };
        changeData.push_back(std::move(tmpEntry));
    }
    return changeData;
}

std::vector<std::string> KVDBWatcher::ConvertToKeys(const std::vector<PRIValue> &values)
{
    std::vector<std::string> keys{};
    for (auto &info : values) {
        auto key = std::get_if<std::string>(&info);
        if (key == nullptr) {
            continue;
        }
        keys.push_back(std::move(*key));
    }
    return keys;
}
} // namespace OHOS::DistributedKv

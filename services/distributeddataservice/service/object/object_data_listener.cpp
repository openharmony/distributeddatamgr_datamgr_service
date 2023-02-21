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

#define LOG_TAG "ObjectDataListener"

#include "object_data_listener.h"
#include "log_print.h"
#include "object_manager.h"

namespace OHOS {
namespace DistributedObject {
ObjectDataListener::ObjectDataListener()
{
}

ObjectDataListener::~ObjectDataListener()
{
}

void ObjectDataListener::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    ZLOGD("ObjectDataListener::OnChange start");
    const auto &insertedDatas = data.GetEntriesInserted();
    const auto &updatedDatas = data.GetEntriesUpdated();
    std::map<std::string, std::vector<uint8_t>> changedData {};
    for (const auto &entry : insertedDatas) {
        std::string key(entry.key.begin(), entry.key.end());
        changedData.insert_or_assign(std::move(key), entry.value);
    }
    for (const auto &entry : updatedDatas) {
        std::string key(entry.key.begin(), entry.key.end());
        changedData.insert_or_assign(std::move(key), entry.value);
    }
    DistributedObject::ObjectStoreManager::GetInstance()->NotifyChange(changedData);
}
}  // namespace DistributedObject
}  // namespace OHOS

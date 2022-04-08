/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "DevResultSet"

#include "device_kvstore_resultset_impl.h"
#include "kvstore_utils.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
using namespace AppDistributedKv;
DeviceKvStoreResultSetImpl::DeviceKvStoreResultSetImpl(
    DistributedDB::KvStoreResultSet *resultSet, DistributedDB::Key keyPrefix)
    : KvStoreResultSetImpl(resultSet, keyPrefix)
{
}

Status DeviceKvStoreResultSetImpl::GetEntry(Entry &entry)
{
    ZLOGD("RS:start");
    Entry tmpEntry;
    Status ret = KvStoreResultSetImpl::GetEntry(tmpEntry);
    if (ret != Status::SUCCESS) {
        return ret;
    }

    auto localDevId = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    if (localDevId.empty()) {
        ZLOGI("RS: localDevId empty");
        return Status::ILLEGAL_STATE;
    }

    std::vector<uint8_t> out;
    out.insert(out.end(), tmpEntry.key.Data().begin() + localDevId.size(), tmpEntry.key.Data().end() - sizeof(int));
    entry.key = out;
    entry.value = tmpEntry.value;
    ZLOGD("RS end.");
    return Status::SUCCESS;
}
} // namespace OHOS::DistributedKv
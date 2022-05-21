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

#define LOG_TAG "KvStoreDataShareResultSet"

#include "constant.h"
#include "log_print.h"
#include "kvstore_datashare_result_set.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
KvStoreDataShareResultSet::KvStoreDataShareResultSet(std::shared_ptr<KvStoreResultSet> kvResultSet)
    :kvResultSet_(kvResultSet) {};

int KvStoreDataShareResultSet::GetRowCount(int32_t &count)
{
    count = Count();
    return count == INVALID_COUNT ? E_ERROR : E_OK;
}

int KvStoreDataShareResultSet::GetAllColumnNames(std::vector<std::string> &columnsName)
{
    columnsName = { "key", "value" };
    return E_OK;
}
bool KvStoreDataShareResultSet::FillBlock(int pos, ResultSetBridge::Writer &writer)
{
    if (kvResultSet_ == nullptr) {
        ZLOGE("kvResultSet_ nullptr");
        return false;
    }
    bool isMoved = kvResultSet_->MoveToPosition(pos);
    if (!isMoved) {
        ZLOGE("MoveToPosition failed");
        return false;
    }
    Entry entry;
    Status status = kvResultSet_->GetEntry(entry);
    if (status != Status::SUCCESS) {
        ZLOGE("GetEntry failed %{public}d", status);
        return false;
    }
    int statusAlloc = writer.AllocRow();
    if (statusAlloc != E_OK) {
        ZLOGE("SharedBlock is full: %{public}d", statusAlloc);
        return false;
    }
    int keyStatus = writer.Write(0, (uint8_t *)&entry.key.Data(), entry.key.Size()); 
    if (keyStatus != E_OK) { 
        ZLOGE("WriteBlob key error: %{public}d", keyStatus);
        return false;
    }
    int valueStatus = writer.Write(1, (uint8_t *)&entry.value.Data(), entry.value.Size());
    if (valueStatus != E_OK) {
        ZLOGE("WriteBlob value error: %{public}d", valueStatus);
        return false;
    }
    return true;
}

int KvStoreDataShareResultSet::Count()
{
    if (kvResultSet_ == nullptr) {
        ZLOGE("kvResultSet_ nullptr");
        return INVALID_COUNT;
    }
    if (resultRowCount != INVALID_COUNT) {
        return resultRowCount;
    }
    int count = kvResultSet_->GetCount();
    if (count < 0) {
        ZLOGE("kvResultSet count invalid: %{public}d", count);
        return INVALID_COUNT;
    }
    resultRowCount = count;
    return count;
}
bool KvStoreDataShareResultSet::OnGo(int32_t start, int32_t target, ResultSetBridge::Writer &writer) 
{
    if ((start < 0) || (target < 0) || (start > target) || (target >= Count())) {
        ZLOGE("nowRowIndex out of line: %{public}d", target);
        return false;
    }
    for (int pos = start; pos <= target; pos++) {
        bool ret = FillBlock(pos + start, writer);
        if (!ret) {
            ZLOGE("nowRowIndex out of line: %{public}d", target);
            return ret;
        }
    }
    return true;
}
} // namespace DistributedKv
} // namespace OHOS
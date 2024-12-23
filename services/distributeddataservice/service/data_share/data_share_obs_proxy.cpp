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

#define LOG_TAG "ObserverProxy"
#include "data_share_obs_proxy.h"
#include "datashare_errno.h"

#include "itypes_util.h"
#include "datashare_itypes_utils.h"
#include "log_print.h"

namespace OHOS {
namespace DataShare {
static constexpr int REQUEST_CODE = 0;
static constexpr int MAX_EXCEEDS = static_cast<size_t>(std::numeric_limits<int>::max());
int RdbObserverProxy::CreateAshmem(RdbChangeNode &changeNode)
{
    OHOS::sptr<Ashmem> memory = Ashmem::CreateAshmem(ASHMEM_NAME, DATA_SIZE_ASHMEM_TRANSFER_LIMIT);
    if (memory == nullptr) {
        ZLOGE("failed to create Ashmem instance.");
        return E_ERROR;
    }
    bool mapRet = memory->MapReadAndWriteAshmem();
    if (!mapRet) {
        ZLOGE("failed to map read and write ashmem, ret=%{public}d", mapRet);
        memory->CloseAshmem();
        return E_ERROR;
    }
    if (changeNode.memory_ != nullptr) {
        ZLOGE(
            "Unknown error: changeNode.memory_ should be null, but something is there %{public}p",
            (void *)changeNode.memory_
        );
        return E_ERROR;
    }
    changeNode.memory_ = memory;
    return E_OK;
}

int RdbObserverProxy::WriteAshmem(RdbChangeNode &changeNode, void *data, int len, int &offset)
{
    if (changeNode.memory_ == nullptr) {
        ZLOGE("changeNode memory is nullptr.");
        return E_ERROR;
    }
    bool writeRet = changeNode.memory_->WriteToAshmem(data, len, offset);
    if (!writeRet) {
        ZLOGE("failed to write into ashmem, ret=%{public}d", writeRet);
        changeNode.memory_->UnmapAshmem();
        changeNode.memory_->CloseAshmem();
        changeNode.memory_ = nullptr;
        return E_ERROR;
    }
    offset += len;
    return E_OK;
}

int RdbObserverProxy::SerializeDataIntoAshmem(RdbChangeNode &changeNode)
{
    if (changeNode.memory_ == nullptr) {
        ZLOGE("changeNode.memory_ is nullptr");
        return E_ERROR;
    }
    // move data
    // simple serialization: [vec_size(int32); str1_len(int32), str1; str2_len(int32), str2; ...],
    // total byte size is recorded in changeNode.size
    int offset = 0;
    // 4 byte for length int
    int intLen = 4;
    size_t uDataSize = changeNode.data_.size();
    if (uDataSize > MAX_EXCEEDS) {
        ZLOGE("changeNode data size exceeds the value of int.");
        return E_ERROR;
    }
    int dataSize = static_cast<int>(uDataSize);
    if (WriteAshmem(changeNode, (void *)&dataSize, intLen, offset) != E_OK) {
        ZLOGE("failed to write data with len %{public}d, offset %{public}d.", intLen, offset);
        return E_ERROR;
    }
    for (int i = 0; i < dataSize; i++) {
        const char *str = changeNode.data_[i].c_str();
        size_t uStrLen = changeNode.data_[i].length();
        if (uStrLen > MAX_EXCEEDS) {
            ZLOGE("string length exceeds the value of int.");
            return E_ERROR;
        }
        int strLen = static_cast<int>(uStrLen);
        // write length int
        if (WriteAshmem(changeNode, (void *)&strLen, intLen, offset) != E_OK) {
            ZLOGE("failed to write data with index %{public}d, len %{public}d, offset %{public}d.", i, intLen, offset);
            return E_ERROR;
        }
        // write str
        if (WriteAshmem(changeNode, (void *)str, strLen, offset) != E_OK) {
            ZLOGE("failed to write data with index %{public}d, len %{public}d, offset %{public}d.", i, strLen, offset);
            return E_ERROR;
        }
    }
    changeNode.size_ = offset;
    return E_OK;
}

int RdbObserverProxy::PrepareRdbChangeNodeData(RdbChangeNode &changeNode)
{
    // If data size is bigger than the limit, move it to the shared memory
    // 4 byte for length int
    int intByteLen = 4;
    int size = intByteLen;
    size_t uDataSize = changeNode.data_.size();
    if (uDataSize > MAX_EXCEEDS) {
        ZLOGE("changeNode data size exceeds the value of int.");
        return E_ERROR;
    }
    int dataSize = static_cast<int>(uDataSize);
    for (int i = 0; i < dataSize; i++) {
        size += intByteLen;
        size_t uStrLen = changeNode.data_[i].length();
        if (uStrLen > MAX_EXCEEDS) {
            ZLOGE("string length exceeds the value of int.");
            return E_ERROR;
        }
        int strLen = static_cast<int>(uStrLen);
        size += strLen;
    }
    if (size > DATA_SIZE_ASHMEM_TRANSFER_LIMIT) {
        ZLOGE("Data to write into ashmem is %{public}d bytes, over 10M.", size);
        return E_ERROR;
    }
    if (size > DATA_SIZE_IPC_TRANSFER_LIMIT) {
        ZLOGD("Data size is over 200k, transfer it by the shared memory");
        if (RdbObserverProxy::CreateAshmem(changeNode) != E_OK) {
            ZLOGE("failed to create ashmem.");
            return E_ERROR;
        }
        if (RdbObserverProxy::SerializeDataIntoAshmem(changeNode) != E_OK) {
            ZLOGE("failed to serialize data into ashmem.");
            return E_ERROR;
        }
        // clear original data spot
        changeNode.data_.clear();
        changeNode.isSharedMemory_ = true;
        ZLOGD("Preparation done. Data size: %{public}d", changeNode.size_);
    }
    return E_OK;
}

void RdbObserverProxy::OnChangeFromRdb(RdbChangeNode &changeNode)
{
    MessageParcel parcel;
    if (!parcel.WriteInterfaceToken(RdbObserverProxy::GetDescriptor())) {
        return;
    }

    if (RdbObserverProxy::PrepareRdbChangeNodeData(changeNode) != E_OK) {
        ZLOGE("failed to prepare RdbChangeNode data.");
        return;
    }

    if (!ITypesUtil::Marshal(parcel, changeNode)) {
        ZLOGE("failed to WriteParcelable changeNode ");
        return;
    }

    MessageParcel reply;
    MessageOption option;
    int32_t result = Remote()->SendRequest(REQUEST_CODE, parcel, reply, option);
    if (result != ERR_NONE) {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
    ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
}

void PublishedDataObserverProxy::OnChangeFromPublishedData(PublishedDataChangeNode &changeNode)
{
    MessageParcel parcel;
    if (!parcel.WriteInterfaceToken(PublishedDataObserverProxy::GetDescriptor())) {
        return;
    }

    if (!ITypesUtil::Marshal(parcel, changeNode)) {
        ZLOGE("failed to WriteParcelable changeNode ");
        return;
    }

    MessageParcel reply;
    MessageOption option;
    int32_t result = Remote()->SendRequest(REQUEST_CODE, parcel, reply, option);
    if (result != ERR_NONE) {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
    ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
}
} // namespace DataShare
} // namespace OHOS
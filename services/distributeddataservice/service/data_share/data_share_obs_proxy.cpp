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
#include "dataproxy_handle_common.h"

#include "itypes_util.h"
#include "datashare_itypes_utils.h"
#include "log_print.h"
#include "log_debug.h"

namespace OHOS {
namespace DataShare {
static constexpr int REQUEST_CODE = 0;
// length of int32 bytes
static constexpr int32_t INT32_BYTE_LEN = static_cast<int32_t>(sizeof(int32_t));
// using DATA_SIZEASHMEM_TRANSFER_LIMIT as the maximum length of changeNode.data_[i]
static constexpr size_t MAX_STR_LEN = static_cast<size_t>(DATA_SIZE_ASHMEM_TRANSFER_LIMIT);
// maximum size of changeNode.data_(all 0-length strings)
static constexpr size_t MAX_DATA_SIZE = MAX_STR_LEN >> 2;
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
        ZLOGE("Unknown error: changeNode.memory_ should be null, but something is there.");
        return E_ERROR;
    }
    changeNode.memory_ = memory;
    return E_OK;
}

int RdbObserverProxy::WriteAshmem(RdbChangeNode &changeNode, void *data, int32_t len, int32_t &offset)
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
    int32_t offset = 0;
    size_t dataSize = changeNode.data_.size();
    // maximum dataSize
    if (dataSize > MAX_DATA_SIZE) {
        ZLOGE("changeNode size:%{public}zu, exceeds the maximum limit.", dataSize);
        return E_ERROR;
    }
    if (WriteAshmem(changeNode, (void *)&dataSize, INT32_BYTE_LEN, offset) != E_OK) {
        ZLOGE("failed to write data with len %{public}d, offset %{public}d.", INT32_BYTE_LEN, offset);
        return E_ERROR;
    }
    for (size_t i = 0; i < dataSize; i++) {
        const char *str = changeNode.data_[i].c_str();
        size_t uStrLen = changeNode.data_[i].length();
        // maximum strLen
        if (uStrLen > MAX_STR_LEN) {
            ZLOGE("string length:%{public}zu, exceeds the maximum limit.", uStrLen);
            return E_ERROR;
        }
        int32_t strLen = static_cast<int32_t>(uStrLen);
        // write length int
        if (WriteAshmem(changeNode, (void *)&strLen, INT32_BYTE_LEN, offset) != E_OK) {
            ZLOGE("failed to write data with index %{public}zu, len %{public}d, offset %{public}d.",
                i, INT32_BYTE_LEN, offset);
            return E_ERROR;
        }
        // write str
        if (WriteAshmem(changeNode, (void *)str, strLen, offset) != E_OK) {
            ZLOGE("failed to write data with index %{public}zu, len %{public}d, offset %{public}d.",
                i, strLen, offset);
            return E_ERROR;
        }
    }
    changeNode.size_ = offset;
    return E_OK;
}

int RdbObserverProxy::PrepareRdbChangeNodeData(RdbChangeNode &changeNode)
{
    // If data size is bigger than the limit, move it to the shared memory
    int32_t size = INT32_BYTE_LEN;
    size_t dataSize = changeNode.data_.size();
    // maximum dataSize
    if (dataSize > MAX_DATA_SIZE) {
        ZLOGE("changeNode size:%{public}zu, exceeds the maximum limit.", dataSize);
        return E_ERROR;
    }
    for (size_t i = 0; i < dataSize; i++) {
        size += INT32_BYTE_LEN;
        size_t uStrLen = changeNode.data_[i].length();
        // maximum strLen
        if (uStrLen > DATA_SIZE_ASHMEM_TRANSFER_LIMIT) {
            ZLOGE("string length:%{public}zu, exceeds the maximum limit.", uStrLen);
            return E_ERROR;
        }
        int32_t strLen = static_cast<int32_t>(uStrLen);
        size += strLen;
    }
    if (size > DATA_SIZE_ASHMEM_TRANSFER_LIMIT) {
        ZLOGE("Data to write into ashmem is %{public}d bytes, over 10M.", size);
        return E_ERROR;
    }
    if (size > DATA_SIZE_IPC_TRANSFER_LIMIT) {
        ZLOGD_MACRO("Data size is over 200k, transfer it by the shared memory");
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
        ZLOGD_MACRO("Preparation done. Data size: %{public}d", changeNode.size_);
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
    ZLOGD("SendRequest ok, retval is %{public}d", reply.ReadInt32());
}

void ProxyDataObserverProxy::OnChangeFromProxyData(std::vector<DataProxyChangeInfo> &changeInfo)
{
    MessageParcel parcel;
    if (!parcel.WriteInterfaceToken(PublishedDataObserverProxy::GetDescriptor())) {
        return;
    }

    if (!ITypesUtil::Marshal(parcel, changeInfo)) {
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
    ZLOGI("SendRequest ok, reply is %{public}d", reply.ReadInt32());
}
} // namespace DataShare
} // namespace OHOS
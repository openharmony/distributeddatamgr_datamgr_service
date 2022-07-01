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

#define LOG_TAG "KvStoreResultSetProxy"

#include "ikvstore_resultset.h"
#include <ipc_skeleton.h>
#include "constant.h"
#include "message_parcel.h"
#include "log_print.h"
#include "itypes_util.h"

namespace OHOS::DistributedKv {
enum {
    GETCOUNT,
    GETPOSITION,
    MOVETOFIRST,
    MOVETOLAST,
    MOVETONEXT,
    MOVETOPREVIOUS,
    MOVE,
    MOVETOPOSITION,
    ISFIRST,
    ISLAST,
    ISBEFOREFIRST,
    ISAFTERLAST,
    GETENTRY,
};

KvStoreResultSetProxy::KvStoreResultSetProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IKvStoreResultSet>(impl)
{}

int KvStoreResultSetProxy::GetCount()
{
    return SendRequest(GETCOUNT);
}

int KvStoreResultSetProxy::GetPosition()
{
    return SendRequest(GETPOSITION);
}

bool KvStoreResultSetProxy::MoveToFirst()
{
    return SendRequestRetBool(MOVETOFIRST);
}

bool KvStoreResultSetProxy::MoveToLast()
{
    return SendRequestRetBool(MOVETOLAST);
}

bool KvStoreResultSetProxy::MoveToNext()
{
    return SendRequestRetBool(MOVETONEXT);
}

bool KvStoreResultSetProxy::MoveToPrevious()
{
    return SendRequestRetBool(MOVETOPREVIOUS);
}

bool KvStoreResultSetProxy::Move(int offset)
{
    MessageParcel data, reply;
    if (!data.WriteInterfaceToken(KvStoreResultSetProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return false;
    }
    bool ret = data.WriteInt32(offset);
    if (!ret) {
        return ret;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(MOVE, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d, code=%d", error, MOVE);
        return false;
    }
    return reply.ReadBool();
}

bool KvStoreResultSetProxy::MoveToPosition(int position)
{
    MessageParcel data, reply;
    if (!data.WriteInterfaceToken(KvStoreResultSetProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return false;
    }
    bool ret = data.WriteInt32(position);
    if (!ret) {
        return ret;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(MOVETOPOSITION, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d, code=%d", error, MOVETOPOSITION);
        return false;
    }
    return reply.ReadBool();
}

bool KvStoreResultSetProxy::IsFirst()
{
    return SendRequestRetBool(ISFIRST);
}

bool KvStoreResultSetProxy::IsLast()
{
    return SendRequestRetBool(ISLAST);
}

bool KvStoreResultSetProxy::IsBeforeFirst()
{
    return SendRequestRetBool(ISBEFOREFIRST);
}

bool KvStoreResultSetProxy::IsAfterLast()
{
    return SendRequestRetBool(ISAFTERLAST);
}

Status KvStoreResultSetProxy::GetEntry(Entry &entry)
{
    MessageParcel data, reply;
    if (!data.WriteInterfaceToken(KvStoreResultSetProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return Status::IPC_ERROR;
    }
    bool ret = reply.SetMaxCapacity(Constant::MAX_IPC_CAPACITY);  // 800K
    if (!ret) {
        ZLOGW("set max capacity failed.");
        return Status::ERROR;
    }

    MessageOption mo { MessageOption::TF_SYNC };
    ZLOGI("start");
    int32_t error = Remote()->SendRequest(GETENTRY, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error is %d", error);
        return Status::IPC_ERROR;
    }

    int32_t status = 0;
    int32_t bufferSize = 0;
    if(!ITypesUtil::Unmarshal(reply, status, bufferSize))
    {
        ZLOGW("read status or bufferSize failed");
        return Status::ERROR;
    }

    if(bufferSize < Constant::SWITCH_RAW_DATA_SIZE)
    {
        if(!ITypesUtil::Unmarshal(reply, entry))
        {
            ZLOGW("read entry failed");
            return Status::ERROR;
        }
        return Status::SUCCESS;
    }
    ZLOGI("getting large data");
    if (bufferSize > static_cast<int64_t>(reply.GetRawDataCapacity())) {
        ZLOGW("bufferSize %d larger than message parcel limit", bufferSize);
        return Status::ERROR;
    }
    status = ITypesUtil::UnmarshalFromBuffer(reply, bufferSize, entry);
    if(status != Status::SUCCESS)
    {
        ZLOGW("read entry failed (%d).", status);
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

int KvStoreResultSetProxy::SendRequest(uint32_t code)
{
    MessageParcel data, reply;
    if (!data.WriteInterfaceToken(KvStoreResultSetProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return -1;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(code, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest returned %d, code=%d", error, code);
        return -1;
    }
    return reply.ReadInt32();
}

bool KvStoreResultSetProxy::SendRequestRetBool(uint32_t code)
{
    MessageParcel data, reply;
    if (!data.WriteInterfaceToken(KvStoreResultSetProxy::GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return false;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int32_t error = Remote()->SendRequest(code, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequestRetBool returned %d, code=%d", error, code);
        return false;
    }
    return reply.ReadBool();
}
int KvStoreResultSetStub::GetEntryOnRemote(MessageParcel &reply)
{
    if (!reply.SetMaxCapacity(Constant::MAX_IPC_CAPACITY)) {
        ZLOGW("set reply MessageParcel capacity failed");
        return -1;
    }

    Entry entry;
    int32_t status = GetEntry(entry);
    int32_t bufferSize = entry.RawEntrySize();
    if(!ITypesUtil::Marshal(reply, status, bufferSize))
    {
        ZLOGW("write status or bufferSize failed.");
        return -1;
    }
    if (bufferSize < Constant::SWITCH_RAW_DATA_SIZE) {
        if (!ITypesUtil::Marshal(reply, entry)) {
            ZLOGW("write entry failed.");
            return -1;
        }
        return 0;
    }
    ZLOGI("getting big data");
    if (bufferSize > static_cast<int64_t>(reply.GetRawDataCapacity())) {
        ZLOGW("bufferSize %d larger than message parcel limit", bufferSize);
        return 0;
    }
    //return status const T &input, int size, MessageParcel &data
    status = ITypesUtil::MarshalToBuffer(entry, bufferSize, reply);
    if (status != Status::SUCCESS) {
        ZLOGW("write entry failed (%{public}d).", status);
        return -1;
    }
    return 0;
}

int KvStoreResultSetStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                          MessageOption &option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    std::u16string descriptor = KvStoreResultSetStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        ZLOGE("local descriptor is not equal to remote");
        return -1;
    }
    switch (code) {
        case GETCOUNT: {
            int count = GetCount();
            bool ret = reply.WriteInt32(count);
            if (!ret) {
                ZLOGW("ResultSet service side GetCount fail.");
            }
            return 0;
        }
        case GETPOSITION: {
            int position = GetPosition();
            bool ret = reply.WriteInt32(position);
            if (!ret) {
                ZLOGW("ResultSet service side GetPosition fail.");
            }
            return 0;
        }
        case MOVETOFIRST: {
            bool isFirst = MoveToFirst();
            bool ret = reply.WriteBool(isFirst);
            if (!ret) {
                ZLOGW("ResultSet service side GetPosition fail.");
            }
            return 0;
        }
        case MOVETOLAST: {
            bool isLast = MoveToLast();
            bool ret = reply.WriteBool(isLast);
            if (!ret) {
                ZLOGW("ResultSet service side GetPosition fail.");
            }
            return 0;
        }
        case MOVETONEXT: {
            bool isNext = MoveToNext();
            bool ret = reply.WriteBool(isNext);
            if (!ret) {
                ZLOGW("ResultSet service side MoveToNext fail.");
            }
            return 0;
        }
        case MOVETOPREVIOUS: {
            bool boolRet = MoveToPrevious();
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side MoveToPrevious fail.");
            }
            return 0;
        }
        case MOVE: {
            uint32_t offset = data.ReadUint32();
            bool boolRet = Move(offset);
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side Move fail.");
            }
            return 0;
        }
        case MOVETOPOSITION: {
            uint32_t position = data.ReadUint32();
            bool boolRet = MoveToPosition(position);
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side MoveToPosition fail.");
            }
            return 0;
        }
        case ISFIRST: {
            bool boolRet = IsFirst();
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side IsFirst fail.");
            }
            return 0;
        }
        case ISLAST: {
            bool boolRet = IsLast();
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side IsLast fail.");
            }
            return 0;
        }
        case ISBEFOREFIRST: {
            bool boolRet = IsBeforeFirst();
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side IsBeforeFirst fail.");
            }
            return 0;
        }
        case ISAFTERLAST: {
            bool boolRet = IsAfterLast();
            bool ret = reply.WriteBool(boolRet);
            if (!ret) {
                ZLOGW("ResultSet service side IsAfterLast fail.");
            }
            return 0;
        }
        case GETENTRY: {
            return GetEntryOnRemote(reply);
        }
        default: {
            ZLOGW("OnRemoteRequest default %{public}u", code);
            MessageOption mo { MessageOption::TF_SYNC };
            return IPCObjectStub::OnRemoteRequest(code, data, reply, mo);
        }
    }
}
} // namespace OHOS::DistributedKv

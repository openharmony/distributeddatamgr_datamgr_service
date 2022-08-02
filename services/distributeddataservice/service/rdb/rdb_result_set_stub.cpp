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

#define LOG_TAG "RdbResultSetStub"

#include <ipc_skeleton.h>
#include "log_print.h"
#include "rdb_result_set_stub.h"

namespace OHOS::DistributedRdb {
int RdbResultSetStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return -1;
    }
    if (code >= 0 && code < CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

bool RdbResultSetStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = RdbResultSetStub::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int32_t RdbResultSetStub::OnGetAllColumnNames(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> columnNames;
    int status = GetAllColumnNames(columnNames);
    if (status != 0) {
        ZLOGE("ResultSet service side GetAllColumnNames failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteStringVector(columnNames)) {
        ZLOGE("Write status or columnNames failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnCount(MessageParcel &data, MessageParcel &reply)
{
    int columnCount = 0;
    int status = GetColumnCount(columnCount);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnCount failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(columnCount)) {
        ZLOGE("Write status or columnCount failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnType(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    NativeRdb::ColumnType columnType;
    int status = GetColumnType(columnIndex, columnType);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnType failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(static_cast<int32_t>(columnType))) {
        ZLOGE("Write status or columnType failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnIndex(MessageParcel &data, MessageParcel &reply)
{
    std::string columnName = data.ReadString();
    int columnIndex;
    int status = GetColumnIndex(columnName, columnIndex);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnIndex failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(columnIndex)) {
        ZLOGE("Write status or columnIndex failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnName(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    std::string columnName;
    int status = GetColumnName(columnIndex, columnName);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnName failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteString(columnName)) {
        ZLOGE("Write status or columnName failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetRowCount(MessageParcel &data, MessageParcel &reply)
{
    int rowCount = 0;
    int status = GetRowCount(rowCount);
    if (status != 0) {
        ZLOGE("ResultSet service side GetRowCount failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(rowCount)) {
        ZLOGE("Write status or rowCount failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetRowIndex(MessageParcel &data, MessageParcel &reply)
{
    int rowIndex = 0;
    int status = GetRowIndex(rowIndex);
    if (status != 0) {
        ZLOGE("ResultSet service side GetRowIndex failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(rowIndex)) {
        ZLOGE("Write status or rowIndex failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoTo(MessageParcel &data, MessageParcel &reply)
{
    int offSet = data.ReadInt32();
    int status = GoTo(offSet);
    if (status != 0) {
        ZLOGE("ResultSet service side GoTo failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToRow(MessageParcel &data, MessageParcel &reply)
{
    int position = data.ReadInt32();
    int status = GoToRow(position);
    if (status != 0) {
        ZLOGE("ResultSet service side GoToRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToFirstRow(MessageParcel &data, MessageParcel &reply)
{
    int status = GoToFirstRow();
    if (status != 0) {
        ZLOGE("ResultSet service side GoToFirstRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToLastRow(MessageParcel &data, MessageParcel &reply)
{
    int status = GoToLastRow();
    if (status != 0) {
        ZLOGE("ResultSet service side GoToLastRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToNextRow(MessageParcel &data, MessageParcel &reply)
{
    int status = GoToNextRow();
    if (status != 0) {
        ZLOGE("ResultSet service side GoToNextRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToPreviousRow(MessageParcel &data, MessageParcel &reply)
{
    int status = GoToPreviousRow();
    if (status != 0) {
        ZLOGE("ResultSet service side GoToPreviousRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsEnded(MessageParcel &data, MessageParcel &reply)
{
    bool isEnded = false;
    int status = IsEnded(isEnded);
    if (status != 0) {
        ZLOGE("ResultSet service side IsEnded failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteBool(isEnded)) {
        ZLOGE("Write status or isEnded failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsStarted(MessageParcel &data, MessageParcel &reply)
{
    bool isStarted = false;
    int status = IsStarted(isStarted);
    if (status != 0) {
        ZLOGE("ResultSet service side IsStarted failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteBool(isStarted)) {
        ZLOGE("Write status or isStarted failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsAtFirstRow(MessageParcel &data, MessageParcel &reply)
{
    bool isAtFirstRow = false;
    int status = IsAtFirstRow(isAtFirstRow);
    if (status != 0) {
        ZLOGE("ResultSet service side IsAtFirstRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteBool(isAtFirstRow)) {
        ZLOGE("Write status or isAtFirstRow failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsAtLastRow(MessageParcel &data, MessageParcel &reply)
{
    bool isAtLastRow = false;
    int status = IsAtLastRow(isAtLastRow);
    if (status != 0) {
        ZLOGE("ResultSet service side IsAtLastRow failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteBool(isAtLastRow)) {
        ZLOGE("Write status or isAtLastRow failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetBlob(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    std::vector<uint8_t> blob;
    int status = GetBlob(columnIndex, blob);
    if (status != 0) {
        ZLOGE("ResultSet service side GetBlob failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteUInt8Vector(blob)) {
        ZLOGE("Write status or blob failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetString(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    std::string value;
    int status = GetString(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetString failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteString(value)) {
        ZLOGE("Write status or string value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetInt(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    int value;
    int status = GetInt(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetInt failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt32(value)) {
        ZLOGE("Write status or int value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetLong(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    int64_t value;
    int status = GetLong(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetLong failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteInt64(value)) {
        ZLOGE("Write status or long value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetDouble(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    double value;
    int status = GetDouble(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetDouble failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteDouble(value)) {
        ZLOGE("Write status or double value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsColumnNull(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex = data.ReadInt32();
    bool isColumnNull;
    int status = IsColumnNull(columnIndex, isColumnNull);
    if (status != 0) {
        ZLOGE("ResultSet service side IsColumnNull failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status) || !reply.WriteBool(isColumnNull)) {
        ZLOGE("Write status or isColumnNull failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsClosed(MessageParcel &data, MessageParcel &reply)
{
    bool isClosed = IsClosed();
    if (!reply.WriteBool(isClosed)) {
        ZLOGE("Write isClosed failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnClose(MessageParcel &data, MessageParcel &reply)
{
    int status = Close();
    if (status != 0) {
        ZLOGE("ResultSet service side Close failed.");
        if (!reply.WriteInt32(status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!reply.WriteInt32(status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}
} // namespace OHOS::DistributedRdb
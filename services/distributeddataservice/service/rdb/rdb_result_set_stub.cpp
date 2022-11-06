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
#include "itypes_util.h"
#include "log_print.h"
#include "rdb_result_set_stub.h"

using ITypesUtil = OHOS::DistributedKv::ITypesUtil;

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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, columnNames)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, columnCount)) {
        ZLOGE("Write status or columnCount failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnType(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    NativeRdb::ColumnType columnType;
    int status = GetColumnType(columnIndex, columnType);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnType failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, static_cast<int32_t>(columnType))) {
        ZLOGE("Write status or columnType failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnIndex(MessageParcel &data, MessageParcel &reply)
{
    std::string columnName;
    ITypesUtil::Unmarshal(data, columnName);
    int columnIndex;
    int status = GetColumnIndex(columnName, columnIndex);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnIndex failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, columnIndex)) {
        ZLOGE("Write status or columnIndex failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnName(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    std::string columnName;
    int status = GetColumnName(columnIndex, columnName);
    if (status != 0) {
        ZLOGE("ResultSet service side GetColumnName failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, columnName)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, rowCount)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, rowIndex)) {
        ZLOGE("Write status or rowIndex failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoTo(MessageParcel &data, MessageParcel &reply)
{
    int offSet;
    ITypesUtil::Unmarshal(data, offSet);
    int status = GoTo(offSet);
    if (status != 0) {
        ZLOGE("ResultSet service side GoTo failed.");
    }

    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToRow(MessageParcel &data, MessageParcel &reply)
{
    int position;
    ITypesUtil::Unmarshal(data, position);
    int status = GoToRow(position);
    if (status != 0) {
        ZLOGE("ResultSet service side GoToRow failed.");
    }

    if (!ITypesUtil::Marshal(reply, status)) {
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
    if (!ITypesUtil::Marshal(reply, status)) {
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
    }

    if (!ITypesUtil::Marshal(reply, status)) {
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
    }

    if (!ITypesUtil::Marshal(reply, status)) {
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
    }

    if (!ITypesUtil::Marshal(reply, status)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, isEnded)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, isStarted)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, isAtFirstRow)) {
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
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, isAtLastRow)) {
        ZLOGE("Write status or isAtLastRow failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetBlob(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    std::vector<uint8_t> blob;
    int status = GetBlob(columnIndex, blob);
    if (status != 0) {
        ZLOGE("ResultSet service side GetBlob failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, blob)) {
        ZLOGE("Write status or blob failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetString(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    std::string value;
    int status = GetString(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetString failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("Write status or string value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetInt(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    int value;
    int status = GetInt(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetInt failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("Write status or int value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetLong(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    int64_t value;
    int status = GetLong(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetLong failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("Write status or long value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetDouble(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    double value;
    int status = GetDouble(columnIndex, value);
    if (status != 0) {
        ZLOGE("ResultSet service side GetDouble failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("Write status or double value failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsColumnNull(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    bool isColumnNull;
    int status = IsColumnNull(columnIndex, isColumnNull);
    if (status != 0) {
        ZLOGE("ResultSet service side IsColumnNull failed.");
        if (!ITypesUtil::Marshal(reply, status)) {
            ZLOGE("Write status failed.");
            return -1;
        }
        return 0;
    }
    if (!ITypesUtil::Marshal(reply, status, isColumnNull)) {
        ZLOGE("Write status or isColumnNull failed.");
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsClosed(MessageParcel &data, MessageParcel &reply)
{
    bool isClosed = IsClosed();
    if (!ITypesUtil::Marshal(reply, isClosed)) {
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
    }

    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed.");
        return -1;
    }
    return 0;
}
} // namespace OHOS::DistributedRdb
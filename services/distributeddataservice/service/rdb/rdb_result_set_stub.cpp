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

namespace OHOS::DistributedRdb {
RdbResultSetStub::RdbResultSetStub(std::shared_ptr<NativeRdb::ResultSet> resultSet) : resultSet_(std::move(resultSet))
{
}

int RdbResultSetStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data) || resultSet_ == nullptr) {
        return -1;
    }
    if (code >= 0 && code < Code::CMD_MAX) {
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
    int status = resultSet_->GetAllColumnNames(columnNames);
    if (!ITypesUtil::Marshal(reply, status, columnNames)) {
        ZLOGE("Write status or columnNames failed, status:%{public}d, columnNames size:%{public}zu.", status,
            columnNames.size());
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnCount(MessageParcel &data, MessageParcel &reply)
{
    int columnCount = 0;
    int status = resultSet_->GetColumnCount(columnCount);
    if (!ITypesUtil::Marshal(reply, status, columnCount)) {
        ZLOGE("Write status or columnCount failed, status:%{public}d, columnCount:%{public}d.", status, columnCount);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetColumnType(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    NativeRdb::ColumnType columnType;
    int status = resultSet_->GetColumnType(columnIndex, columnType);
    if (!ITypesUtil::Marshal(reply, status, static_cast<int32_t>(columnType))) {
        ZLOGE("Write status or columnType failed, status:%{public}d, columnIndex:%{public}d, columnType:%{public}d.",
            status, columnIndex, static_cast<int32_t>(columnType));
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetRowCount(MessageParcel &data, MessageParcel &reply)
{
    int rowCount = 0;
    int status = resultSet_->GetRowCount(rowCount);
    if (!ITypesUtil::Marshal(reply, status, rowCount)) {
        ZLOGE("Write status or rowCount failed, status:%{public}d, rowCount:%{public}d.", status, rowCount);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetRowIndex(MessageParcel &data, MessageParcel &reply)
{
    int rowIndex = 0;
    int status = resultSet_->GetRowIndex(rowIndex);
    if (!ITypesUtil::Marshal(reply, status, rowIndex)) {
        ZLOGE("Write status or rowIndex failed, status:%{public}d, rowIndex:%{public}d.", status, rowIndex);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoTo(MessageParcel &data, MessageParcel &reply)
{
    int offSet;
    ITypesUtil::Unmarshal(data, offSet);
    int status = resultSet_->GoTo(offSet);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d, offSet:%{public}d.", status, offSet);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToRow(MessageParcel &data, MessageParcel &reply)
{
    int position;
    ITypesUtil::Unmarshal(data, position);
    int status = resultSet_->GoToRow(position);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d, position:%{public}d.", status, position);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToFirstRow(MessageParcel &data, MessageParcel &reply)
{
    int status = resultSet_->GoToFirstRow();
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d.", status);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToLastRow(MessageParcel &data, MessageParcel &reply)
{
    int status = resultSet_->GoToLastRow();
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d.", status);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToNextRow(MessageParcel &data, MessageParcel &reply)
{
    int status = resultSet_->GoToNextRow();
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d.", status);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGoToPreviousRow(MessageParcel &data, MessageParcel &reply)
{
    int status = resultSet_->GoToPreviousRow();
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d.", status);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsEnded(MessageParcel &data, MessageParcel &reply)
{
    bool isEnded = false;
    int status = resultSet_->IsEnded(isEnded);
    if (!ITypesUtil::Marshal(reply, status, isEnded)) {
        ZLOGE("Write status or isEnded failed, status:%{public}d, isEnded:%{public}d.", status, isEnded);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsStarted(MessageParcel &data, MessageParcel &reply)
{
    bool isStarted = false;
    int status = resultSet_->IsStarted(isStarted);
    if (!ITypesUtil::Marshal(reply, status, isStarted)) {
        ZLOGE("Write status or isStarted failed, status:%{public}d, isStarted:%{public}d.", status, isStarted);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsAtFirstRow(MessageParcel &data, MessageParcel &reply)
{
    bool isAtFirstRow = false;
    int status = resultSet_->IsAtFirstRow(isAtFirstRow);
    if (!ITypesUtil::Marshal(reply, status, isAtFirstRow)) {
        ZLOGE("Write status or isAtFirstRow failed, status:%{public}d, isAtFirstRow:%{public}d", status, isAtFirstRow);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsAtLastRow(MessageParcel &data, MessageParcel &reply)
{
    bool isAtLastRow = false;
    int status = resultSet_->IsAtLastRow(isAtLastRow);
    if (!ITypesUtil::Marshal(reply, status, isAtLastRow)) {
        ZLOGE("Write status or isAtLastRow failed, status:%{public}d, isAtLastRow:%{public}d.", status, isAtLastRow);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGet(MessageParcel &data, MessageParcel &reply)
{
    int columnIndex;
    ITypesUtil::Unmarshal(data, columnIndex);
    NativeRdb::ValueObject value;
    int status = resultSet_->Get(columnIndex, value);
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("status:%{public}d, columnIndex:%{public}d", status, columnIndex);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnGetSize(MessageParcel &data, MessageParcel &reply)
{
    int col;
    ITypesUtil::Unmarshal(data, col);
    size_t value = 0;
    int status = resultSet_->GetSize(col, value);
    if (!ITypesUtil::Marshal(reply, status, value)) {
        ZLOGE("failed, status:%{public}d, col:%{public}d, size:%{public}zu", status, col, value);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnIsClosed(MessageParcel &data, MessageParcel &reply)
{
    bool isClosed = resultSet_->IsClosed();
    if (!ITypesUtil::Marshal(reply, isClosed)) {
        ZLOGE("Write isClosed failed, isClosed:%{public}d.", isClosed);
        return -1;
    }
    return 0;
}

int32_t RdbResultSetStub::OnClose(MessageParcel &data, MessageParcel &reply)
{
    int status = resultSet_->Close();
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Write status failed, status:%{public}d.", status);
        return -1;
    }
    return 0;
}
} // namespace OHOS::DistributedRdb
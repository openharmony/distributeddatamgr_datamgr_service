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

#ifndef DISTRIBUTED_RDB_RDB_RESULT_SET_STUB_H
#define DISTRIBUTED_RDB_RDB_RESULT_SET_STUB_H

#include <iremote_stub.h>
#include "irdb_result_set.h"
#include "result_set.h"

namespace OHOS::DistributedRdb {
class RdbResultSetStub : public IRemoteStub<IRdbResultSet> {
public:
    using Code = NativeRdb::RemoteResultSet::Code;
    explicit RdbResultSetStub(std::shared_ptr<NativeRdb::ResultSet> resultSet);
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    int32_t OnGetAllColumnNames(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetColumnCount(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetColumnType(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetColumnIndex(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetColumnName(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetRowCount(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetRowIndex(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoTo(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoToRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoToFirstRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoToLastRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoToNextRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnGoToPreviousRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsEnded(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsStarted(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsAtFirstRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsAtLastRow(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetBlob(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetString(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetInt(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetLong(MessageParcel &data, MessageParcel &reply);
    int32_t OnGetDouble(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsColumnNull(MessageParcel &data, MessageParcel &reply);
    int32_t OnIsClosed(MessageParcel &data, MessageParcel &reply);
    int32_t OnClose(MessageParcel &data, MessageParcel &reply);

    static bool CheckInterfaceToken(MessageParcel &data);
    using RequestHandle = int (RdbResultSetStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[Code::CMD_MAX] = {
        [Code::CMD_GET_ALL_COLUMN_NAMES] = &RdbResultSetStub::OnGetAllColumnNames,
        [Code::CMD_GET_COLUMN_COUNT] = &RdbResultSetStub::OnGetColumnCount,
        [Code::CMD_GET_COLUMN_TYPE] = &RdbResultSetStub::OnGetColumnType,
        [Code::CMD_GET_COLUMN_INDEX] = &RdbResultSetStub::OnGetColumnIndex,
        [Code::CMD_GET_COLUMN_NAME] = &RdbResultSetStub::OnGetColumnName,
        [Code::CMD_GET_ROW_COUNT] = &RdbResultSetStub::OnGetRowCount,
        [Code::CMD_GET_ROW_INDEX] = &RdbResultSetStub::OnGetRowIndex,
        [Code::CMD_GO_TO] = &RdbResultSetStub::OnGoTo,
        [Code::CMD_GO_TO_ROW] = &RdbResultSetStub::OnGoToRow,
        [Code::CMD_GO_TO_FIRST_ROW] = &RdbResultSetStub::OnGoToFirstRow,
        [Code::CMD_GO_TO_LAST_ROW] = &RdbResultSetStub::OnGoToLastRow,
        [Code::CMD_GO_TO_NEXT_ROW] = &RdbResultSetStub::OnGoToNextRow,
        [Code::CMD_GO_TO_PREV_ROW] = &RdbResultSetStub::OnGoToPreviousRow,
        [Code::CMD_IS_ENDED_ROW] = &RdbResultSetStub::OnIsEnded,
        [Code::CMD_IS_STARTED_ROW] = &RdbResultSetStub::OnIsStarted,
        [Code::CMD_IS_AT_FIRST_ROW] = &RdbResultSetStub::OnIsAtFirstRow,
        [Code::CMD_IS_AT_LAST_ROW] = &RdbResultSetStub::OnIsAtLastRow,
        [Code::CMD_GET_BLOB] = &RdbResultSetStub::OnGetBlob,
        [Code::CMD_GET_STRING] = &RdbResultSetStub::OnGetString,
        [Code::CMD_GET_INT] = &RdbResultSetStub::OnGetInt,
        [Code::CMD_GET_LONG] = &RdbResultSetStub::OnGetLong,
        [Code::CMD_GET_DOUBLE] = &RdbResultSetStub::OnGetDouble,
        [Code::CMD_IS_COLUMN_NULL] = &RdbResultSetStub::OnIsColumnNull,
        [Code::CMD_IS_CLOSED] = &RdbResultSetStub::OnIsClosed,
        [Code::CMD_CLOSE] = &RdbResultSetStub::OnClose
    };
    std::shared_ptr<NativeRdb::ResultSet> resultSet_;
};
} // namespace OHOS::DistributedRdb
#endif // DISTRIBUTED_RDB_RDB_RESULT_SET_STUB_H

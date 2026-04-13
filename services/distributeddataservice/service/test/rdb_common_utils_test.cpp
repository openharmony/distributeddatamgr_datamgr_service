/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "rdb_common_utils.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "store/general_value.h"

#include <gtest/gtest.h>
using namespace testing::ext;
using namespace testing;
using namespace OHOS::DistributedRdb;
using RdbStatus = OHOS::DistributedRdb::RdbStatus;
namespace OHOS::Test {
class RdbCommonUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};
void RdbCommonUtilsTest::SetUpTestCase(void) {};
void RdbCommonUtilsTest::TearDownTestCase(void) {};
void RdbCommonUtilsTest::SetUp() {};
void RdbCommonUtilsTest::TearDown() {};
/**
* @tc.name: GetSearchableTables_001
* @tc.desc: get searchable tables from RdbChangedData.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbCommonUtilsTest, GetSearchableTables_001, TestSize.Level0)
{
    RdbChangedData changedData;
    changedData.tableData = {
        { "table1", { .isTrackedDataChange = true } },
        { "table2", { .isP2pSyncDataChange = true } },
        { "table3", { .isP2pSyncDataChange = true } },
        { "table4", { .isTrackedDataChange = true } },
    };
    auto searchableTables = RdbCommonUtils::GetSearchableTables(changedData);
    std::vector<std::string> tagTables = { "table1", "table4" };
    EXPECT_EQ(searchableTables, tagTables);
}

/**
* @tc.name: GetP2PTables_001
* @tc.desc: get p2p sync tables from RdbChangedData.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbCommonUtilsTest, GetP2PTables_001, TestSize.Level0)
{
    RdbChangedData changedData;
    changedData.tableData = {
        { "table1", { .isTrackedDataChange = true } },
        { "table2", { .isP2pSyncDataChange = true } },
        { "table3", { .isP2pSyncDataChange = true } },
        { "table4", { .isTrackedDataChange = true } },
    };
    auto p2pTables = RdbCommonUtils::GetP2PTables(changedData);
    std::vector<std::string> tagTables = { "table2", "table3" };
    EXPECT_EQ(p2pTables, tagTables);
}

/**
* @tc.name: Convert_001
* @tc.desc: convert DistributedRdb Reference to DistributedData::Reference.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(RdbCommonUtilsTest, Convert_001, TestSize.Level0)
{
    std::vector<Reference> references = {
        { "srcTable", "tagTable", { {"uuid", "srcId"} } },
        { "srcTable1", "tagTable1", { {"uuid", "srcId"} } },
        { "srcTable2", "tagTable2", { {"uuid", "srcId"} } },
    };
    auto relations = RdbCommonUtils::Convert(references);
    std::vector<DistributedData::Reference> tagRelations = {
        { "srcTable", "tagTable", { { "uuid", "srcId" } } },
        { "srcTable1", "tagTable1", { { "uuid", "srcId" } } },
        { "srcTable2", "tagTable2", { { "uuid", "srcId" } } },
    };
    EXPECT_EQ(relations.size(), tagRelations.size());
    for (int i = 0; i < tagRelations.size(); ++i) {
        EXPECT_EQ(relations[i].sourceTable, tagRelations[i].sourceTable);
        EXPECT_EQ(relations[i].targetTable, tagRelations[i].targetTable);
        EXPECT_EQ(relations[i].refFields, tagRelations[i].refFields);
    }
}

/**
* @tc.name: ConvertNativeRdbStatus_001
* @tc.desc: ConvertNativeRdbStatus error code test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbCommonUtilsTest, ConvertNativeRdbStatus_001, TestSize.Level1)
{
    int32_t status = NativeRdb::E_SQLITE_BUSY;
    int32_t ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_BUSY);
    status = NativeRdb::E_DATABASE_BUSY;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_BUSY);
    status = NativeRdb::E_SQLITE_LOCKED;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_BUSY);
    status = NativeRdb::E_INVALID_ARGS;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_INVALID_ARGS);
    status = NativeRdb::E_INVALID_ARGS_NEW;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_INVALID_ARGS);
    status = NativeRdb::E_ALREADY_CLOSED;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_ALREADY_CLOSED);
    status = NativeRdb::E_SQLITE_CORRUPT;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_DB_CORRUPT);
    status = NativeRdb::E_SQLITE_SCHEMA;
    ret = RdbCommonUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, DistributedData::GeneralError::E_ERROR);
}

/**
* @tc.name: RdbCommonUtilsTest002
* @tc.desc: ConvertGeneralRdbStatus error code test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbCommonUtilsTest, RdbCommonUtilsTest002, TestSize.Level1)
{
    int32_t ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_BUSY);
    EXPECT_EQ(ret, RdbStatus::RDB_SQLITE_BUSY);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_OK);
    EXPECT_EQ(ret, RdbStatus::RDB_OK);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(ret, RdbStatus::RDB_INVALID_ARGS);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_DB_ERROR);
    EXPECT_EQ(ret, RdbStatus::RDB_SQLITE_ERROR);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_TABLE_NOT_FOUND);
    EXPECT_EQ(ret, RdbStatus::RDB_SQLITE_ERROR);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_NOT_SUPPORT);
    EXPECT_EQ(ret, RdbStatus::RDB_NOT_SUPPORT);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_DB_CORRUPT);
    EXPECT_EQ(ret, RdbStatus::RDB_SQLITE_CORRUPT);
    ret = RdbCommonUtils::ConvertGeneralRdbStatus(DistributedData::GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(ret, RdbStatus::RDB_ERROR);
}

/**
* @tc.name: GetInterfaceErrorString_001
* @tc.desc: Test GetInterfaceErrorString with all interface error codes.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbCommonUtilsTest, GetInterfaceErrorString_001, TestSize.Level1)
{
    // Test all defined interface error codes
    std::vector<DBStatus> interfaceErrorCodes = {
        DBStatus::NOT_SUPPORT,
        DBStatus::INVALID_ARGS,
        DBStatus::DISTRIBUTED_SCHEMA_NOT_FOUND,
        DBStatus::DISTRIBUTED_SCHEMA_CHANGED,
        DBStatus::INVALID_QUERY_FORMAT,
        DBStatus::BUSY,
        DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB,
    };

    for (const auto& errorCode : interfaceErrorCodes) {
        auto errorInfo = RdbCommonUtils::GetInterfaceErrorString(errorCode);
        // Verify the returned dbStatus matches the input
        EXPECT_EQ(errorInfo.dbStatus, errorCode);
        // Verify message is not null for defined error codes
        EXPECT_NE(errorInfo.message, nullptr);
        // Verify syncResultCode is not the default value (0)
        EXPECT_NE(errorInfo.syncResultCode, static_cast<SyncResultCode>(0));
    }

    // Test unknown error code - should return default ErrorInfo
    auto unknownErrorInfo = RdbCommonUtils::GetInterfaceErrorString(static_cast<DBStatus>(9999));
    EXPECT_EQ(unknownErrorInfo.dbStatus, static_cast<DBStatus>(0));
    EXPECT_EQ(unknownErrorInfo.syncResultCode, static_cast<SyncResultCode>(0));
    EXPECT_EQ(unknownErrorInfo.message, nullptr);
}

/**
* @tc.name: GetCallbackErrorString_001
* @tc.desc: Test GetCallbackErrorString with all callback error codes.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbCommonUtilsTest, GetCallbackErrorString_001, TestSize.Level1)
{
    // Test all defined callback error codes
    std::vector<DBStatus> callbackErrorCodes = {
        DBStatus::TABLE_FIELD_MISMATCH,
        DBStatus::DISTRIBUTED_SCHEMA_MISMATCH,
        DBStatus::SECURITY_OPTION_CHECK_ERROR,
        DBStatus::BUSY,
        DBStatus::PERMISSION_CHECK_FORBID_SYNC,
        DBStatus::TIME_OUT,
        DBStatus::INVALID_QUERY_FORMAT,
        DBStatus::INVALID_QUERY_FIELD,
        DBStatus::DISTRIBUTED_SCHEMA_CHANGED,
        DBStatus::NO_PERMISSION,
        DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB,
        DBStatus::NEED_CORRECT_TARGET_USER,
        DBStatus::CONSTRAINT,
        DBStatus::COMM_FAILURE,
        DBStatus::NOT_SUPPORT,
    };

    for (const auto& errorCode : callbackErrorCodes) {
        auto errorInfo = RdbCommonUtils::GetCallbackErrorString(errorCode);
        // Verify the returned dbStatus matches the input
        EXPECT_EQ(errorInfo.dbStatus, errorCode);
        // Verify message is not null for defined error codes
        EXPECT_NE(errorInfo.message, nullptr);
        // Verify syncResultCode is not the default value (0)
        EXPECT_NE(errorInfo.syncResultCode, static_cast<SyncResultCode>(0));
    }

    // Test unknown error code - should return default ErrorInfo
    auto unknownErrorInfo = RdbCommonUtils::GetCallbackErrorString(static_cast<DBStatus>(9999));
    EXPECT_EQ(unknownErrorInfo.dbStatus, static_cast<DBStatus>(0));
    EXPECT_EQ(unknownErrorInfo.syncResultCode, static_cast<SyncResultCode>(0));
    EXPECT_EQ(unknownErrorInfo.message, nullptr);
}
}
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

#include <gtest/gtest.h>

#include <string>

/*
 * White-box test: include the source file directly to access
 * file-scope static functions (IsValidIdentifier, BuildSql, GetMigratedData).
 * The BUILD.gn target must NOT also list object_asset_machine.cpp as a
 * separate source to avoid multiple-definition errors.
 */
#include "object_asset_machine.cpp"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedObject;
namespace OHOS::Test {

class ObjectAssetMachineInternalTest : public testing::Test {
public:
    void SetUp() override;
    void TearDown() override;

protected:
    AssetBindInfo validBindInfo_;
    Asset asset_;
};

void ObjectAssetMachineInternalTest::SetUp()
{
    asset_ = Asset{
        .name = "test_name",
        .uri = "file:://test/asset1.jpg",
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    validBindInfo_ = AssetBindInfo{
        .storeName = "store_test",
        .tableName = "table_test",
        .primaryKey = VBucket{ { "id", 111 } },
        .field = "attachment",
        .assetName = "asset1.jpg",
    };
}

void ObjectAssetMachineInternalTest::TearDown()
{
}

/* ==================== IsValidIdentifier branch coverage ==================== */

/**
 * @tc.name: IsValidIdentifier_EmptyString_ReturnsFalse
 * @tc.desc: Empty string should be rejected (name.empty() branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_EmptyString_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidIdentifier(""));
}

/**
 * @tc.name: IsValidIdentifier_FirstCharDigit_ReturnsFalse
 * @tc.desc: Identifier starting with a digit is invalid (i==0, !isAlpha && !isUnderscore).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_FirstCharDigit_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidIdentifier("1abc"));
    EXPECT_FALSE(IsValidIdentifier("9"));
    EXPECT_FALSE(IsValidIdentifier("0field"));
}

/**
 * @tc.name: IsValidIdentifier_FirstCharSpecialChar_ReturnsFalse
 * @tc.desc: Identifier starting with a special character is invalid (i==0, !isAlpha && !isUnderscore).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_FirstCharSpecialChar_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidIdentifier("-abc"));
    EXPECT_FALSE(IsValidIdentifier(";DROP"));
    EXPECT_FALSE(IsValidIdentifier(" field"));
    EXPECT_FALSE(IsValidIdentifier("' OR 1=1"));
}

/**
 * @tc.name: IsValidIdentifier_FirstCharUppercase_ReturnsTrue
 * @tc.desc: Identifier starting with an uppercase letter is valid (i==0, isAlpha).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_FirstCharUppercase_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("Table"));
    EXPECT_TRUE(IsValidIdentifier("A"));
    EXPECT_TRUE(IsValidIdentifier("MY_TABLE"));
}

/**
 * @tc.name: IsValidIdentifier_FirstCharLowercase_ReturnsTrue
 * @tc.desc: Identifier starting with a lowercase letter is valid (i==0, isAlpha).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_FirstCharLowercase_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("field"));
    EXPECT_TRUE(IsValidIdentifier("a"));
    EXPECT_TRUE(IsValidIdentifier("myColumn"));
}

/**
 * @tc.name: IsValidIdentifier_FirstCharUnderscore_ReturnsTrue
 * @tc.desc: Identifier starting with an underscore is valid (i==0, isUnderscore).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_FirstCharUnderscore_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("_field"));
    EXPECT_TRUE(IsValidIdentifier("_"));
    EXPECT_TRUE(IsValidIdentifier("_table_123"));
}

/**
 * @tc.name: IsValidIdentifier_SubsequentDigit_ReturnsTrue
 * @tc.desc: Subsequent characters can be digits (i>0, isDigit).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_SubsequentDigit_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("col1"));
    EXPECT_TRUE(IsValidIdentifier("table007"));
    EXPECT_TRUE(IsValidIdentifier("_0"));
}

/**
 * @tc.name: IsValidIdentifier_SubsequentSpecialChar_ReturnsFalse
 * @tc.desc: Subsequent special characters are invalid (i>0, !isAlpha && !isDigit && !isUnderscore).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_SubsequentSpecialChar_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidIdentifier("field;"));
    EXPECT_FALSE(IsValidIdentifier("col-umn"));
    EXPECT_FALSE(IsValidIdentifier("table "));
    EXPECT_FALSE(IsValidIdentifier("a'OR"));
    EXPECT_FALSE(IsValidIdentifier("field.name"));
}

/**
 * @tc.name: IsValidIdentifier_SqlInjectionAttempt_ReturnsFalse
 * @tc.desc: SQL injection payloads should be rejected by the identifier whitelist.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_SqlInjectionAttempt_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidIdentifier("field; DROP TABLE t"));
    EXPECT_FALSE(IsValidIdentifier("1;1"));
    EXPECT_FALSE(IsValidIdentifier("' OR '1'='1"));
    EXPECT_FALSE(IsValidIdentifier("table--"));
    EXPECT_FALSE(IsValidIdentifier("col/*comment*/"));
}

/**
 * @tc.name: IsValidIdentifier_SingleChar_ReturnsTrue
 * @tc.desc: A single valid character should pass (loop runs once with i==0).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_SingleChar_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("a"));
    EXPECT_TRUE(IsValidIdentifier("Z"));
    EXPECT_TRUE(IsValidIdentifier("_"));
}

/**
 * @tc.name: IsValidIdentifier_MultipleValidChars_ReturnsTrue
 * @tc.desc: Multiple valid characters should pass the full loop.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidIdentifier_MultipleValidChars_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidIdentifier("attachment"));
    EXPECT_TRUE(IsValidIdentifier("_my_table_2"));
    EXPECT_TRUE(IsValidIdentifier("ABC123def"));
}

/* ==================== IsValidQualifiedName branch coverage ==================== */

/**
 * @tc.name: IsValidQualifiedName_EmptyString_ReturnsFalse
 * @tc.desc: Empty string should be rejected (name.empty() branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_EmptyString_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName(""));
}

/**
 * @tc.name: IsValidQualifiedName_SimpleIdentifier_ReturnsTrue
 * @tc.desc: A name without dots is delegated to IsValidIdentifier (no-dot branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_SimpleIdentifier_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidQualifiedName("field"));
    EXPECT_TRUE(IsValidQualifiedName("my_column"));
    EXPECT_TRUE(IsValidQualifiedName("_col1"));
}

/**
 * @tc.name: IsValidQualifiedName_SimpleInvalidIdentifier_ReturnsFalse
 * @tc.desc: A name without dots but invalid is rejected by the trailing IsValidIdentifier call.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_SimpleInvalidIdentifier_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("1field"));
    EXPECT_FALSE(IsValidQualifiedName("col;"));
    EXPECT_FALSE(IsValidQualifiedName(" field"));
}

/**
 * @tc.name: IsValidQualifiedName_SingleDot_ReturnsTrue
 * @tc.desc: table.field format with both parts valid should pass (loop body + trailing IsValidIdentifier).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_SingleDot_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidQualifiedName("table.field"));
    EXPECT_TRUE(IsValidQualifiedName("t.f"));
    EXPECT_TRUE(IsValidQualifiedName("my_table.my_column"));
}

/**
 * @tc.name: IsValidQualifiedName_MultipleDots_ReturnsTrue
 * @tc.desc: db.table.field format with all parts valid should pass (multiple loop iterations).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_MultipleDots_ReturnsTrue, TestSize.Level0)
{
    EXPECT_TRUE(IsValidQualifiedName("db.table.field"));
    EXPECT_TRUE(IsValidQualifiedName("a.b.c.d"));
}

/**
 * @tc.name: IsValidQualifiedName_LeadingDot_ReturnsFalse
 * @tc.desc: A leading dot triggers pos == start on first iteration.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_LeadingDot_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName(".field"));
    EXPECT_FALSE(IsValidQualifiedName(".table.field"));
}

/**
 * @tc.name: IsValidQualifiedName_TrailingDot_ReturnsFalse
 * @tc.desc: A trailing dot triggers start >= name.size() after the loop.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_TrailingDot_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("table."));
    EXPECT_FALSE(IsValidQualifiedName("table.field."));
}

/**
 * @tc.name: IsValidQualifiedName_ConsecutiveDots_ReturnsFalse
 * @tc.desc: Consecutive dots trigger pos == start on a subsequent iteration.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_ConsecutiveDots_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("table..field"));
    EXPECT_FALSE(IsValidQualifiedName("a..b.c"));
}

/**
 * @tc.name: IsValidQualifiedName_InvalidFirstPart_ReturnsFalse
 * @tc.desc: An invalid first segment is rejected by IsValidIdentifier inside the loop.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_InvalidFirstPart_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("1table.field"));
    EXPECT_FALSE(IsValidQualifiedName("tab le.field"));
    EXPECT_FALSE(IsValidQualifiedName("col;.field"));
}

/**
 * @tc.name: IsValidQualifiedName_InvalidLastPart_ReturnsFalse
 * @tc.desc: An invalid last segment is rejected by the trailing IsValidIdentifier call.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_InvalidLastPart_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("table.1field"));
    EXPECT_FALSE(IsValidQualifiedName("table.field;"));
    EXPECT_FALSE(IsValidQualifiedName("table.col "));
}

/**
 * @tc.name: IsValidQualifiedName_InvalidMiddlePart_ReturnsFalse
 * @tc.desc: An invalid middle segment is rejected inside the loop on the next iteration.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_InvalidMiddlePart_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("db.1table.field"));
    EXPECT_FALSE(IsValidQualifiedName("a.b .c"));
}

/**
 * @tc.name: IsValidQualifiedName_SqlInjectionWithDot_ReturnsFalse
 * @tc.desc: SQL injection payloads containing dots should be rejected.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, IsValidQualifiedName_SqlInjectionWithDot_ReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(IsValidQualifiedName("table.field; DROP TABLE t"));
    EXPECT_FALSE(IsValidQualifiedName("t.f' OR '1'='1"));
    EXPECT_FALSE(IsValidQualifiedName("table.--"));
}

/* ==================== BuildSql branch coverage ==================== */

/**
 * @tc.name: BuildSql_InvalidField_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when field is an invalid identifier.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_InvalidField_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "field; DROP";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/**
 * @tc.name: BuildSql_InvalidTableName_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when tableName is an invalid identifier (field is valid).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_InvalidTableName_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.tableName = "table 123";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/**
 * @tc.name: BuildSql_InvalidPrimaryKey_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when a primary key column name is invalid
 * (field and tableName are valid).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_InvalidPrimaryKey_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{ { "col;injection", 1 } };
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
}

/**
 * @tc.name: BuildSql_EmptyField_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when field is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_EmptyField_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
}

/**
 * @tc.name: BuildSql_EmptyTableName_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when tableName is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_EmptyTableName_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.tableName = "";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
}

/**
 * @tc.name: BuildSql_AllValid_ReturnsNonEmptySql
 * @tc.desc: BuildSql returns a valid SQL string when all identifiers are valid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_AllValid_ReturnsNonEmptySql, TestSize.Level0)
{
    Values args;
    auto sql = BuildSql(validBindInfo_, args);
    EXPECT_FALSE(sql.empty());
    EXPECT_NE(sql.find("SELECT"), std::string::npos);
    EXPECT_NE(sql.find(validBindInfo_.field), std::string::npos);
    EXPECT_NE(sql.find(validBindInfo_.tableName), std::string::npos);
    EXPECT_EQ(args.size(), validBindInfo_.primaryKey.size());
}

/**
 * @tc.name: BuildSql_AllValid_SqlContainsPrimaryKeyColumns
 * @tc.desc: BuildSql includes primary key column names in the WHERE clause.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_AllValid_SqlContainsPrimaryKeyColumns, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{ { "user_id", 1 }, { "item_id", 2 } };
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_FALSE(sql.empty());
    EXPECT_NE(sql.find("user_id"), std::string::npos);
    EXPECT_NE(sql.find("item_id"), std::string::npos);
    EXPECT_EQ(args.size(), 2);
}

/**
 * @tc.name: BuildSql_EmptyPrimaryKey_ReturnsEmpty
 * @tc.desc: BuildSql returns empty string when primaryKey is empty (new empty-check branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_EmptyPrimaryKey_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{};
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/**
 * @tc.name: BuildSql_QualifiedField_ReturnsNonEmptySql
 * @tc.desc: BuildSql accepts table.field format for the field column (IsValidQualifiedName pass).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_QualifiedField_ReturnsNonEmptySql, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "table_test.attachment";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_FALSE(sql.empty());
    EXPECT_NE(sql.find("table_test.attachment"), std::string::npos);
    EXPECT_EQ(args.size(), validBindInfo_.primaryKey.size());
}

/**
 * @tc.name: BuildSql_QualifiedPrimaryKey_ReturnsNonEmptySql
 * @tc.desc: BuildSql accepts table.field format for primary key columns (IsValidQualifiedName pass).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_QualifiedPrimaryKey_ReturnsNonEmptySql, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{ { "table_test.id", 111 } };
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_FALSE(sql.empty());
    EXPECT_NE(sql.find("table_test.id"), std::string::npos);
    EXPECT_EQ(args.size(), 1);
}

/**
 * @tc.name: BuildSql_QualifiedFieldAndPrimaryKey_ReturnsNonEmptySql
 * @tc.desc: BuildSql accepts table.field for both field and primary key simultaneously.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_QualifiedFieldAndPrimaryKey_ReturnsNonEmptySql, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "t.attachment";
    bindInfo.primaryKey = VBucket{ { "t.user_id", 1 }, { "t.item_id", 2 } };
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_FALSE(sql.empty());
    EXPECT_NE(sql.find("t.attachment"), std::string::npos);
    EXPECT_NE(sql.find("t.user_id"), std::string::npos);
    EXPECT_NE(sql.find("t.item_id"), std::string::npos);
    EXPECT_EQ(args.size(), 2);
}

/**
 * @tc.name: BuildSql_InvalidQualifiedField_ReturnsEmpty
 * @tc.desc: BuildSql returns empty when field is a qualified name with an invalid part.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_InvalidQualifiedField_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "table.1field";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/**
 * @tc.name: BuildSql_InvalidQualifiedPrimaryKey_ReturnsEmpty
 * @tc.desc: BuildSql returns empty when a primary key column is a qualified name with an invalid part.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_InvalidQualifiedPrimaryKey_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{ { "table.1key", 1 } };
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
}

/**
 * @tc.name: BuildSql_QualifiedFieldWithLeadingDot_ReturnsEmpty
 * @tc.desc: BuildSql returns empty when field has a leading dot.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_QualifiedFieldWithLeadingDot_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = ".attachment";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/**
 * @tc.name: BuildSql_QualifiedFieldWithTrailingDot_ReturnsEmpty
 * @tc.desc: BuildSql returns empty when field has a trailing dot.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, BuildSql_QualifiedFieldWithTrailingDot_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "attachment.";
    Values args;
    auto sql = BuildSql(bindInfo, args);
    EXPECT_TRUE(sql.empty());
    EXPECT_TRUE(args.empty());
}

/* ==================== GetMigratedData branch coverage ==================== */

/**
 * @tc.name: GetMigratedData_InvalidIdentifier_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when BuildSql fails due to invalid identifiers
 * (new sql.empty() early-return branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_InvalidIdentifier_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "field; DROP";
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GetMigratedData_InvalidTableName_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when tableName is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_InvalidTableName_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.tableName = "table 1";
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GetMigratedData_InvalidPrimaryKey_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when primary key column is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_InvalidPrimaryKey_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{ { "bad;key", 1 } };
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GetMigratedData_EmptyField_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when field is empty string.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_EmptyField_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "";
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GetMigratedData_EmptyPrimaryKey_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when primaryKey is empty (BuildSql empty-check branch).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_EmptyPrimaryKey_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.primaryKey = VBucket{};
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: GetMigratedData_QualifiedFieldInvalid_ReturnsEmpty
 * @tc.desc: GetMigratedData returns empty result when qualified field name is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(ObjectAssetMachineInternalTest, GetMigratedData_QualifiedFieldInvalid_ReturnsEmpty, TestSize.Level0)
{
    AssetBindInfo bindInfo = validBindInfo_;
    bindInfo.field = "table.1field";
    AutoCache::Store nullStore;
    auto result = GetMigratedData(nullStore, bindInfo, asset_);
    EXPECT_TRUE(result.empty());
}

} // namespace OHOS::Test

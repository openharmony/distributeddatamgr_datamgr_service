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

#define LOG_TAG "SchemaMetaCompatibleTest"
#include <gtest/gtest.h>

#include "cloud/schema_meta.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class SchemaMetaCompatibleTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
 * @tc.name: CompatibleConstraint_Marshal_BothTrue_Success
 * @tc.desc: Marshal CompatibleConstraint with both notNull and hasDefault set to true.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_Marshal_BothTrue_Success, TestSize.Level0)
{
    CompatibleConstraint constraint;
    constraint.notNull = true;
    constraint.hasDefault = true;

    Serializable::json node;
    ASSERT_TRUE(constraint.Marshal(node));
    EXPECT_EQ(node["notNull"], true);
    EXPECT_EQ(node["hasDefault"], true);
    EXPECT_EQ(Serializable::Marshall(constraint), to_string(node));
}

/**
 * @tc.name: CompatibleConstraint_Unmarshal_SameAsOriginal
 * @tc.desc: Unmarshal CompatibleConstraint from json node and verify equality with original.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_Unmarshal_SameAsOriginal, TestSize.Level0)
{
    CompatibleConstraint constraint1;
    constraint1.notNull = true;
    constraint1.hasDefault = false;

    Serializable::json node;
    constraint1.Marshal(node);

    CompatibleConstraint constraint2;
    ASSERT_TRUE(constraint2.Unmarshal(node));
    EXPECT_EQ(constraint1, constraint2);
}

/**
 * @tc.name: CompatibleConstraint_OperatorEqual_SameValues
 * @tc.desc: Two CompatibleConstraint instances with same values should be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_OperatorEqual_SameValues, TestSize.Level0)
{
    CompatibleConstraint constraint1;
    constraint1.notNull = true;
    constraint1.hasDefault = true;

    CompatibleConstraint constraint2;
    constraint2.notNull = true;
    constraint2.hasDefault = true;

    EXPECT_EQ(constraint1, constraint2);
}

/**
 * @tc.name: CompatibleConstraint_OperatorEqual_DifferentNotNull
 * @tc.desc: Two CompatibleConstraint instances with different notNull should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_OperatorEqual_DifferentNotNull, TestSize.Level0)
{
    CompatibleConstraint constraint1;
    constraint1.notNull = true;
    constraint1.hasDefault = false;

    CompatibleConstraint constraint2;
    constraint2.notNull = false;
    constraint2.hasDefault = false;

    EXPECT_EQ(constraint1, constraint2);
}

/**
 * @tc.name: CompatibleConstraint_OperatorEqual_DifferentHasDefault
 * @tc.desc: Two CompatibleConstraint instances with different hasDefault should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_OperatorEqual_DifferentHasDefault, TestSize.Level0)
{
    CompatibleConstraint constraint1;
    constraint1.notNull = false;
    constraint1.hasDefault = true;

    CompatibleConstraint constraint2;
    constraint2.notNull = false;
    constraint2.hasDefault = false;

    EXPECT_EQ(constraint1, constraint2);
}

/**
 * @tc.name: CompatibleConstraint_DefaultValue_BothFalse
 * @tc.desc: Default constructed CompatibleConstraint should have both fields as false.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatibleConstraint_DefaultValue_BothFalse, TestSize.Level0)
{
    CompatibleConstraint constraint;
    EXPECT_FALSE(constraint.notNull);
    EXPECT_FALSE(constraint.hasDefault);
}

/**
 * @tc.name: FieldsPolicy_Marshal_WithConstraints_Success
 * @tc.desc: Marshal FieldsPolicy with columnName and compatibleConstraints.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_Marshal_WithConstraints_Success, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = true;
    cc.hasDefault = false;

    FieldsPolicy policy;
    policy.columnName = "test_column";
    policy.compatibleConstraints.push_back(cc);

    Serializable::json node;
    ASSERT_TRUE(policy.Marshal(node));
    EXPECT_EQ(node["columnName"], "test_column");
    EXPECT_EQ(Serializable::Marshall(policy), to_string(node));
}

/**
 * @tc.name: FieldsPolicy_Unmarshal_SameAsOriginal
 * @tc.desc: Unmarshal FieldsPolicy from json node and verify equality with original.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_Unmarshal_SameAsOriginal, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = true;
    cc.hasDefault = true;

    FieldsPolicy policy1;
    policy1.columnName = "test_column";
    policy1.compatibleConstraints.push_back(cc);

    Serializable::json node;
    policy1.Marshal(node);

    FieldsPolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: FieldsPolicy_OperatorEqual_SameValues
 * @tc.desc: Two FieldsPolicy instances with same values should be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_OperatorEqual_SameValues, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = true;
    cc.hasDefault = false;

    FieldsPolicy policy1;
    policy1.columnName = "col_A";
    policy1.compatibleConstraints.push_back(cc);

    FieldsPolicy policy2;
    policy2.columnName = "col_A";
    policy2.compatibleConstraints.push_back(cc);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: FieldsPolicy_OperatorEqual_DifferentColumnName
 * @tc.desc: Two FieldsPolicy instances with different columnName should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_OperatorEqual_DifferentColumnName, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = false;
    cc.hasDefault = true;

    FieldsPolicy policy1;
    policy1.columnName = "col_A";
    policy1.compatibleConstraints.push_back(cc);

    FieldsPolicy policy2;
    policy2.columnName = "col_B";
    policy2.compatibleConstraints.push_back(cc);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: FieldsPolicy_OperatorEqual_DifferentConstraints
 * @tc.desc: Two FieldsPolicy instances with different compatibleConstraints should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_OperatorEqual_DifferentConstraints, TestSize.Level0)
{
    CompatibleConstraint cc1;
    cc1.notNull = true;
    cc1.hasDefault = false;

    CompatibleConstraint cc2;
    cc2.notNull = false;
    cc2.hasDefault = true;

    FieldsPolicy policy1;
    policy1.columnName = "col_A";
    policy1.compatibleConstraints.push_back(cc1);

    FieldsPolicy policy2;
    policy2.columnName = "col_A";
    policy2.compatibleConstraints.push_back(cc2);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: FieldsPolicy_EmptyConstraints_MarshalUnmarshal
 * @tc.desc: Marshal/Unmarshal FieldsPolicy with empty compatibleConstraints vector.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_EmptyConstraints_MarshalUnmarshal, TestSize.Level0)
{
    FieldsPolicy policy1;
    policy1.columnName = "empty_col";
    policy1.compatibleConstraints = {};

    Serializable::json node;
    ASSERT_TRUE(policy1.Marshal(node));

    FieldsPolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
    EXPECT_EQ(policy2.columnName, "empty_col");
    EXPECT_EQ(policy2.compatibleConstraints.size(), 0u);
}

/**
 * @tc.name: FieldsPolicy_MultipleConstraints_MarshalUnmarshal
 * @tc.desc: Marshal/Unmarshal FieldsPolicy with multiple CompatibleConstraint elements.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, FieldsPolicy_MultipleConstraints_MarshalUnmarshal, TestSize.Level0)
{
    CompatibleConstraint cc1;
    cc1.notNull = true;
    cc1.hasDefault = false;

    CompatibleConstraint cc2;
    cc2.notNull = false;
    cc2.hasDefault = true;

    CompatibleConstraint cc3;
    cc3.notNull = true;
    cc3.hasDefault = true;

    FieldsPolicy policy1;
    policy1.columnName = "multi_col";
    policy1.compatibleConstraints.push_back(cc1);
    policy1.compatibleConstraints.push_back(cc2);
    policy1.compatibleConstraints.push_back(cc3);

    Serializable::json node;
    ASSERT_TRUE(policy1.Marshal(node));

    FieldsPolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
    EXPECT_EQ(policy2.compatibleConstraints.size(), 3u);
}

/**
 * @tc.name: CompatiblePolicy_Marshal_WithFieldsPolicy_Success
 * @tc.desc: Marshal CompatiblePolicy with tableName and fieldsPolicy.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_Marshal_WithFieldsPolicy_Success, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = true;
    cc.hasDefault = false;

    FieldsPolicy fp;
    fp.columnName = "field_col";
    fp.compatibleConstraints.push_back(cc);

    CompatiblePolicy policy;
    policy.tableName = "test_table";
    policy.fieldsPolicy.push_back(fp);

    Serializable::json node;
    ASSERT_TRUE(policy.Marshal(node));
    EXPECT_EQ(node["tableName"], "test_table");
    EXPECT_EQ(Serializable::Marshall(policy), to_string(node));
}

/**
 * @tc.name: CompatiblePolicy_Unmarshal_SameAsOriginal
 * @tc.desc: Unmarshal CompatiblePolicy from json node and verify equality with original.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_Unmarshal_SameAsOriginal, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = true;
    cc.hasDefault = true;

    FieldsPolicy fp;
    fp.columnName = "policy_col";
    fp.compatibleConstraints.push_back(cc);

    CompatiblePolicy policy1;
    policy1.tableName = "tbl_A";
    policy1.fieldsPolicy.push_back(fp);

    Serializable::json node;
    policy1.Marshal(node);

    CompatiblePolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: CompatiblePolicy_OperatorEqual_SameValues
 * @tc.desc: Two CompatiblePolicy instances with same values should be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_OperatorEqual_SameValues, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = false;
    cc.hasDefault = true;

    FieldsPolicy fp;
    fp.columnName = "same_col";
    fp.compatibleConstraints.push_back(cc);

    CompatiblePolicy policy1;
    policy1.tableName = "tbl_same";
    policy1.fieldsPolicy.push_back(fp);

    CompatiblePolicy policy2;
    policy2.tableName = "tbl_same";
    policy2.fieldsPolicy.push_back(fp);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: CompatiblePolicy_OperatorEqual_DifferentTableName
 * @tc.desc: Two CompatiblePolicy instances with different tableName should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_OperatorEqual_DifferentTableName, TestSize.Level0)
{
    CompatibleConstraint cc;
    cc.notNull = false;
    cc.hasDefault = true;

    FieldsPolicy fp;
    fp.columnName = "same_col";
    fp.compatibleConstraints.push_back(cc);

    CompatiblePolicy policy1;
    policy1.tableName = "tbl_A";
    policy1.fieldsPolicy.push_back(fp);

    CompatiblePolicy policy2;
    policy2.tableName = "tbl_B";
    policy2.fieldsPolicy.push_back(fp);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: CompatiblePolicy_OperatorEqual_DifferentFieldsPolicy
 * @tc.desc: Two CompatiblePolicy instances with different fieldsPolicy should not be equal.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_OperatorEqual_DifferentFieldsPolicy, TestSize.Level0)
{
    CompatibleConstraint cc1;
    cc1.notNull = true;
    cc1.hasDefault = false;

    CompatibleConstraint cc2;
    cc2.notNull = false;
    cc2.hasDefault = true;

    FieldsPolicy fp1;
    fp1.columnName = "col_A";
    fp1.compatibleConstraints.push_back(cc1);

    FieldsPolicy fp2;
    fp2.columnName = "col_B";
    fp2.compatibleConstraints.push_back(cc2);

    CompatiblePolicy policy1;
    policy1.tableName = "tbl_same";
    policy1.fieldsPolicy.push_back(fp1);

    CompatiblePolicy policy2;
    policy2.tableName = "tbl_same";
    policy2.fieldsPolicy.push_back(fp2);

    EXPECT_EQ(policy1, policy2);
}

/**
 * @tc.name: CompatiblePolicy_EmptyFieldsPolicy_MarshalUnmarshal
 * @tc.desc: Marshal/Unmarshal CompatiblePolicy with empty fieldsPolicy vector.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_EmptyFieldsPolicy_MarshalUnmarshal, TestSize.Level0)
{
    CompatiblePolicy policy1;
    policy1.tableName = "empty_table";
    policy1.fieldsPolicy = {};

    Serializable::json node;
    ASSERT_TRUE(policy1.Marshal(node));

    CompatiblePolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
    EXPECT_EQ(policy2.tableName, "empty_table");
    EXPECT_EQ(policy2.fieldsPolicy.size(), 0u);
}

/**
 * @tc.name: CompatiblePolicy_NestedFullHierarchy_MarshalUnmarshal
 * @tc.desc: Marshal/Unmarshal full three-level nested hierarchy (Policy->FieldsPolicy->Constraint).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(SchemaMetaCompatibleTest, CompatiblePolicy_NestedFullHierarchy_MarshalUnmarshal, TestSize.Level0)
{
    CompatibleConstraint cc1;
    cc1.notNull = true;
    cc1.hasDefault = false;

    CompatibleConstraint cc2;
    cc2.notNull = false;
    cc2.hasDefault = true;

    FieldsPolicy fp1;
    fp1.columnName = "col_alpha";
    fp1.compatibleConstraints.push_back(cc1);

    FieldsPolicy fp2;
    fp2.columnName = "col_beta";
    fp2.compatibleConstraints.push_back(cc2);
    fp2.compatibleConstraints.push_back(cc1);

    CompatiblePolicy policy1;
    policy1.tableName = "full_table";
    policy1.fieldsPolicy.push_back(fp1);
    policy1.fieldsPolicy.push_back(fp2);

    Serializable::json node;
    ASSERT_TRUE(policy1.Marshal(node));

    CompatiblePolicy policy2;
    ASSERT_TRUE(policy2.Unmarshal(node));
    EXPECT_EQ(policy1, policy2);
    EXPECT_EQ(policy2.tableName, "full_table");
    EXPECT_EQ(policy2.fieldsPolicy.size(), 2u);
    EXPECT_EQ(policy2.fieldsPolicy[0].columnName, "col_alpha");
    EXPECT_EQ(policy2.fieldsPolicy[0].compatibleConstraints.size(), 1u);
    EXPECT_EQ(policy2.fieldsPolicy[1].columnName, "col_beta");
    EXPECT_EQ(policy2.fieldsPolicy[1].compatibleConstraints.size(), 2u);
    EXPECT_EQ(policy2.fieldsPolicy[1].compatibleConstraints[0].notNull, false);
    EXPECT_EQ(policy2.fieldsPolicy[1].compatibleConstraints[0].hasDefault, true);
    EXPECT_EQ(policy2.fieldsPolicy[1].compatibleConstraints[1].notNull, true);
    EXPECT_EQ(policy2.fieldsPolicy[1].compatibleConstraints[1].hasDefault, false);
}
} // namespace OHOS::Test

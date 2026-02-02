/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <vector>
#include <functional>

#include "message_parcel.h"
#include "parcel.h"
#include "rdb_types_utils.h"
#include "itypes_util.h"

using namespace testing::ext;
using namespace testing;
using namespace OHOS;
using namespace OHOS::DistributedRdb;
using namespace OHOS::ITypesUtil;

namespace OHOS {
namespace Test {

class RdbTypesUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RdbTypesUtilsTest::SetUpTestCase(void)
{
}

void RdbTypesUtilsTest::TearDownTestCase(void)
{
}

void RdbTypesUtilsTest::SetUp(void)
{
}

void RdbTypesUtilsTest::TearDown(void)
{
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_NotifyConfig_Basic
 * @tc.desc: 测试 NotifyConfig 基础反序列化功能
 * @tc.type: FUNC
 * @tc.require: AR000VHI9H
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_NotifyConfig_Basic, TestSize.Level1)
{
    // Arrange - 先序列化一个 NotifyConfig
    NotifyConfig originalConfig;
    originalConfig.delay_ = 1000;
    originalConfig.isFull_ = true;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

    // Act - 反序列化
    NotifyConfig unmarshalledConfig;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledConfig);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledConfig.delay_, originalConfig.delay_);
    EXPECT_EQ(unmarshalledConfig.isFull_, originalConfig.isFull_);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_NotifyConfig_DifferentValues
 * @tc.desc: 测试 NotifyConfig 不同参数值的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_NotifyConfig_DifferentValues, TestSize.Level1)
{
    struct TestCase {
        uint32_t delay;
        bool isFull;
    };

    std::vector<TestCase> testCases = {
        {0, false},           // 最小 delay, 不全量
        {100, false},          // 正常 delay
        {1000, true},          // 正常 delay, 全量
        {UINT32_MAX, false},   // 最大 delay
        {5000, true}           // 大 delay, 全量
    };

    for (const auto& testCase : testCases) {
        // Arrange
        NotifyConfig originalConfig;
        originalConfig.delay_ = testCase.delay;
        originalConfig.isFull_ = testCase.isFull;

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

        // Act
        NotifyConfig unmarshalledConfig;
        bool result = ITypesUtil::Unmarshal(parcel, unmarshalledConfig);

        // Assert
        EXPECT_EQ(result, true)
            << "Failed for delay=" << testCase.delay
            << ", isFull=" << testCase.isFull;
        EXPECT_EQ(unmarshalledConfig.delay_, testCase.delay);
        EXPECT_EQ(unmarshalledConfig.isFull_, testCase.isFull);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Option_Complete
 * @tc.desc: 测试 Option 完整反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Option_Complete, TestSize.Level1)
{
    // Arrange
    Option originalOption;
    originalOption.mode = 1;
    originalOption.seqNum = 12345;
    originalOption.isAsync = true;
    originalOption.isAutoSync = false;
    originalOption.isCompensation = true;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

    // Act
    Option unmarshalledOption;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledOption);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledOption.mode, originalOption.mode);
    EXPECT_EQ(unmarshalledOption.seqNum, originalOption.seqNum);
    EXPECT_EQ(unmarshalledOption.isAsync, originalOption.isAsync);
    EXPECT_EQ(unmarshalledOption.isAutoSync, originalOption.isAutoSync);
    EXPECT_EQ(unmarshalledOption.isCompensation, originalOption.isCompensation);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Option_FlagsCombinations
 * @tc.desc: 测试 Option 所有标志位组合的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Option_FlagsCombinations, TestSize.Level1)
{
    // 测试 8 种标志组合
    for (int isAsync = 0; isAsync <= 1; isAsync++) {
        for (int isAutoSync = 0; isAutoSync <= 1; isAutoSync++) {
            for (int isCompensation = 0; isCompensation <= 1; isCompensation++) {
                // Arrange
                Option originalOption;
                originalOption.mode = 0;
                originalOption.seqNum = 100;
                originalOption.isAsync = static_cast<bool>(isAsync);
                originalOption.isAutoSync = static_cast<bool>(isAutoSync);
                originalOption.isCompensation = static_cast<bool>(isCompensation);

                MessageParcel parcel;
                ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

                // Act
                Option unmarshalledOption;
                bool result = ITypesUtil::Unmarshal(parcel, unmarshalledOption);

                // Assert
                EXPECT_TRUE(result)
                    << "Failed for flags: isAsync=" << isAsync
                    << ", isAutoSync=" << isAutoSync
                    << ", isCompensation=" << isCompensation;
                EXPECT_EQ(unmarshalledOption.isAsync, static_cast<bool>(isAsync));
                EXPECT_EQ(unmarshalledOption.isAutoSync, static_cast<bool>(isAutoSync));
                EXPECT_EQ(unmarshalledOption.isCompensation, static_cast<bool>(isCompensation));
            }
        }
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_SubOption_AllModes
 * @tc.desc: 测试 SubOption 所有订阅模式的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_SubOption_AllModes, TestSize.Level1)
{
    std::vector<SubscribeMode> modes = {
        SubscribeMode::CLOUD,
        SubscribeMode::LOCAL,
        SubscribeMode::REMOTE
    };

    for (auto mode : modes) {
        // Arrange
        SubOption originalOption;
        originalOption.mode = mode;

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

        // Act
        SubOption unmarshalledOption;
        bool result = ITypesUtil::Unmarshal(parcel, unmarshalledOption);

        // Assert
        EXPECT_EQ(result, true)
            << "Failed for SubscribeMode: " << static_cast<int>(mode);
        EXPECT_EQ(unmarshalledOption.mode, mode);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_SubOption_ModeValue
 * @tc.desc: 测试 SubOption 模式值转换的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_SubOption_ModeValue, TestSize.Level1)
{
    // 测试不同的模式值
    std::vector<int32_t> modeValues = {0, 1, 2, 10, 100, -1};

    for (int32_t modeValue : modeValues) {
        // Arrange
        SubOption originalOption;
        originalOption.mode = static_cast<SubscribeMode>(modeValue);

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

        // Act
        SubOption unmarshalledOption;
        bool result = ITypesUtil::Unmarshal(parcel, unmarshalledOption);

        // Assert
        EXPECT_EQ(result, true)
            << "Failed for mode value: " << modeValue;
        EXPECT_EQ(static_cast<int32_t>(unmarshalledOption.mode), modeValue);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_SingleTable
 * @tc.desc: 测试单表 RdbChangedData 反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_SingleTable, TestSize.Level1)
{
    // Arrange
    RdbChangedData originalChangedData;
    RdbChangeProperties properties;
    properties.isTrackedDataChange = true;
    properties.isP2pSyncDataChange = false;
    originalChangedData.tableData["test_table"] = properties;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    // Act
    RdbChangedData unmarshalledChangedData;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledChangedData);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());
    EXPECT_EQ(unmarshalledChangedData.tableData["test_table"].isTrackedDataChange,
              originalChangedData.tableData["test_table"].isTrackedDataChange);
    EXPECT_EQ(unmarshalledChangedData.tableData["test_table"].isP2pSyncDataChange,
              originalChangedData.tableData["test_table"].isP2pSyncDataChange);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_MultipleTables
 * @tc.desc: 测试多表 RdbChangedData 反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_MultipleTables, TestSize.Level1)
{
    // Arrange
    RdbChangedData originalChangedData;

    // 添加 5 个表的数据
    for (int i = 0; i < 5; i++) {
        std::string tableName = "table_" + std::to_string(i);
        RdbChangeProperties properties;
        properties.isTrackedDataChange = (i % 2 == 0);
        properties.isP2pSyncDataChange = (i % 3 == 0);
        originalChangedData.tableData[tableName] = properties;
    }

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    // Act
    RdbChangedData unmarshalledChangedData;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledChangedData);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());

    for (const auto& [tableName, properties] : originalChangedData.tableData) {
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isTrackedDataChange,
                  properties.isTrackedDataChange);
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isP2pSyncDataChange,
                  properties.isP2pSyncDataChange);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_EmptyTableData
 * @tc.desc: 测试空 tableData 的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_EmptyTableData, TestSize.Level1)
{
    // Arrange
    RdbChangedData originalChangedData;
    // tableData 为空

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    // Act
    RdbChangedData unmarshalledChangedData;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledChangedData);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), 0);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbProperties_PropertyCombinations
 * @tc.desc: 测试 RdbProperties 属性组合的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbProperties_PropertyCombinations, TestSize.Level1)
{
    struct TestCase {
        bool isTrackedDataChange;
        bool isP2pSyncDataChange;
    };

    std::vector<TestCase> testCases = {
        {false, false},  // 都不跟踪
        {true, false},   // 仅跟踪
        {false, true},   // 仅 P2P
        {true, true}     // 都启用
    };

    for (const auto& testCase : testCases) {
        // Arrange
        RdbProperties originalProperties;
        originalProperties.isTrackedDataChange = testCase.isTrackedDataChange;
        originalProperties.isP2pSyncDataChange = testCase.isP2pSyncDataChange;

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalProperties));

        // Act
        RdbProperties unmarshalledProperties;
        bool result = ITypesUtil::Unmarshal(parcel, unmarshalledProperties);

        // Assert
        EXPECT_EQ(result, true)
            << "Failed for tracked=" << testCase.isTrackedDataChange
            << ", p2p=" << testCase.isP2pSyncDataChange;
        EXPECT_EQ(unmarshalledProperties.isTrackedDataChange, testCase.isTrackedDataChange);
        EXPECT_EQ(unmarshalledProperties.isP2pSyncDataChange, testCase.isP2pSyncDataChange);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Reference_Basic
 * @tc.desc: 测试 Reference 基础反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Reference_Basic, TestSize.Level1)
{
    // Arrange
    Reference originalRef;
    originalRef.sourceTable = "source_table";
    originalRef.targetTable = "target_table";
    originalRef.refFields.insert({"field1", "field1_value"});
    originalRef.refFields.insert({"field2", "field2_value"});
    originalRef.refFields.insert({"field3", "field3_value"});

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));

    // Act
    Reference unmarshalledRef;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledRef);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledRef.sourceTable, originalRef.sourceTable);
    EXPECT_EQ(unmarshalledRef.targetTable, originalRef.targetTable);
    EXPECT_EQ(unmarshalledRef.refFields.size(), originalRef.refFields.size());
    EXPECT_EQ(unmarshalledRef.refFields["field1"], "field1_value");
    EXPECT_EQ(unmarshalledRef.refFields["field2"], "field2_value");
    EXPECT_EQ(unmarshalledRef.refFields["field3"], "field3_value");
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Reference_MultipleFields
 * @tc.desc: 测试多字段引用的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Reference_MultipleFields, TestSize.Level1)
{
    // Arrange
    Reference originalRef;
    originalRef.sourceTable = "src";
    originalRef.targetTable = "tgt";

    // 添加不同数量的字段
    std::vector<size_t> fieldCounts = {0, 1, 5, 10};

    for (size_t fieldCount : fieldCounts) {
        originalRef.refFields.clear();
        for (size_t i = 0; i < fieldCount; i++) {
            std::string fieldName = "field_" + std::to_string(i);
            originalRef.refFields.insert({fieldName, fieldName + "_value"});
        }

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));

        // Act
        Reference unmarshalledRef;
        bool result = ITypesUtil::Unmarshal(parcel, unmarshalledRef);

        // Assert
        EXPECT_EQ(result, true)
            << "Failed with " << fieldCount << " fields";
        EXPECT_EQ(unmarshalledRef.sourceTable, originalRef.sourceTable);
        EXPECT_EQ(unmarshalledRef.targetTable, originalRef.targetTable);
        EXPECT_EQ(unmarshalledRef.refFields.size(), originalRef.refFields.size());
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_StatReporter_Complete
 * @tc.desc: 测试 StatReporter 完整反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_StatReporter_Complete, TestSize.Level1)
{
    // Arrange
    StatReporter originalReporter;
    originalReporter.statType = 3;
    originalReporter.bundleName = "com.example.app";
    originalReporter.storeName = "test.db";
    originalReporter.subType = 1;
    originalReporter.costTime = 250;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    // Act
    StatReporter unmarshalledReporter;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledReporter);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledReporter.statType, originalReporter.statType);
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
    EXPECT_EQ(unmarshalledReporter.storeName, originalReporter.storeName);
    EXPECT_EQ(unmarshalledReporter.subType, originalReporter.subType);
    EXPECT_EQ(unmarshalledReporter.costTime, originalReporter.costTime);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_StatReporter_LongStrings
 * @tc.desc: 测试长字符串的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_StatReporter_LongStrings, TestSize.Level1)
{
    // Arrange
    StatReporter originalReporter;
    originalReporter.statType = 1;
    originalReporter.bundleName = std::string(1000, 'a');  // 1000字符的包名
    originalReporter.storeName = std::string(1000, 'b');   // 1000字符的数据库名
    originalReporter.subType = 0;
    originalReporter.costTime = 999999;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    // Act
    StatReporter unmarshalledReporter;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledReporter);

    // Assert
    EXPECT_EQ(result, true);
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
    EXPECT_EQ(unmarshalledReporter.storeName, originalReporter.storeName);
    EXPECT_EQ(unmarshalledReporter.costTime, originalReporter.costTime);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_MaxDelayValue
 * @tc.desc: 测试最大 delay 值的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_MaxDelayValue, TestSize.Level1)
{
    // Arrange
    NotifyConfig originalConfig;
    originalConfig.delay_ = UINT32_MAX;
    originalConfig.isFull_ = true;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

    // Act
    NotifyConfig unmarshalledConfig;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledConfig);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(unmarshalledConfig.delay_, UINT32_MAX);
    EXPECT_EQ(unmarshalledConfig.isFull_, true);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_MaxSeqNum
 * @tc.desc: 测试最大序列号的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_MaxSeqNum, TestSize.Level1)
{
    // Arrange
    Option originalOption;
    originalOption.mode = 0;
    originalOption.seqNum = INT32_MAX;
    originalOption.isAsync = true;
    originalOption.isAutoSync = true;
    originalOption.isCompensation = true;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

    // Act
    Option unmarshalledOption;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledOption);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(unmarshalledOption.seqNum, INT32_MAX);
    EXPECT_EQ(unmarshalledOption.isAsync, true);
    EXPECT_EQ(unmarshalledOption.isAutoSync, true);
    EXPECT_EQ(unmarshalledOption.isCompensation, true);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_EmptyRefFields
 * @tc.desc: 测试空引用字段的反序列化
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_EmptyRefFields, TestSize.Level1)
{
    // Arrange
    Reference originalRef;
    originalRef.sourceTable = "src";
    originalRef.targetTable = "tgt";
    originalRef.refFields.clear();

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));

    // Act
    Reference unmarshalledRef;
    bool result = ITypesUtil::Unmarshal(parcel, unmarshalledRef);

    // Assert
    EXPECT_TRUE(result);
    EXPECT_EQ(unmarshalledRef.sourceTable, "src");
    EXPECT_EQ(unmarshalledRef.targetTable, "tgt");
    EXPECT_EQ(unmarshalledRef.refFields.size(), 0);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Integration_MultipleTypesInOneParcel
 * @tc.desc: 测试在一个 Parcel 中反序列化多种类型
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Integration_MultipleTypesInOneParcel, TestSize.Level1)
{
    MessageParcel parcel;

    // 1. 序列化 NotifyConfig
    NotifyConfig originalNotifyConfig;
    originalNotifyConfig.delay_ = 1000;
    originalNotifyConfig.isFull_ = true;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalNotifyConfig));

    // 2. 序列化 Option
    Option originalOption;
    originalOption.mode = 0;
    originalOption.seqNum = 12345;
    originalOption.isAsync = true;
    originalOption.isAutoSync = false;
    originalOption.isCompensation = false;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

    // 3. 序列化 SubOption
    SubOption originalSubOption;
    originalSubOption.mode = SubscribeMode::CLOUD;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalSubOption));

    // 4. 序列化 RdbChangedData
    RdbChangedData originalChangedData;
    RdbChangeProperties changeProps;
    changeProps.isTrackedDataChange = true;
    changeProps.isP2pSyncDataChange = false;
    originalChangedData.tableData["table"] = changeProps;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    // 5. 序列化 RdbProperties
    RdbProperties originalProperties;
    originalProperties.isTrackedDataChange = true;
    originalProperties.isP2pSyncDataChange = false;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalProperties));

    // 6. 序列化 Reference
    Reference originalReference;
    originalReference.sourceTable = "source";
    originalReference.targetTable = "target";
    originalReference.refFields.insert({"f1", "f1_value"});
    originalReference.refFields.insert({"f2", "f2_value"});
    originalReference.refFields.insert({"f3", "f3_value"});
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReference));

    // 7. 序列化 StatReporter
    StatReporter originalReporter;
    originalReporter.statType = 1;
    originalReporter.bundleName = "com.example.app";
    originalReporter.storeName = "test.db";
    originalReporter.subType = 0;
    originalReporter.costTime = 100;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    // 反序列化并验证
    NotifyConfig unmarshalledNotifyConfig;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledNotifyConfig));
    EXPECT_EQ(unmarshalledNotifyConfig.delay_, originalNotifyConfig.delay_);
    EXPECT_EQ(unmarshalledNotifyConfig.isFull_, originalNotifyConfig.isFull_);

    Option unmarshalledOption;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption));
    EXPECT_EQ(unmarshalledOption.seqNum, originalOption.seqNum);

    SubOption unmarshalledSubOption;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledSubOption));
    EXPECT_EQ(unmarshalledSubOption.mode, originalSubOption.mode);

    RdbChangedData unmarshalledChangedData;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledChangedData));
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());

    RdbProperties unmarshalledProperties;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledProperties));
    EXPECT_EQ(unmarshalledProperties.isTrackedDataChange, originalProperties.isTrackedDataChange);

    Reference unmarshalledReference;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReference));
    EXPECT_EQ(unmarshalledReference.sourceTable, originalReference.sourceTable);

    StatReporter unmarshalledReporter;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReporter));
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_AllModifiedInterfaces
 * @tc.desc: 回归测试所有修改的接口
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_AllModifiedInterfaces, TestSize.Level1)
{
    using TestFunc = std::function<bool()>;

    std::vector<std::pair<std::string, TestFunc>> tests = {
        {"NotifyConfig::Unmarshalling", []() {
            NotifyConfig originalConfig;
            originalConfig.delay_ = 1000;
            originalConfig.isFull_ = true;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalConfig)) {
                return false;
            }

            NotifyConfig unmarshalledConfig;
            return ITypesUtil::Unmarshal(parcel, unmarshalledConfig) &&
                   unmarshalledConfig.delay_ == originalConfig.delay_ &&
                   unmarshalledConfig.isFull_ == originalConfig.isFull_;
        }},
        {"Option::Unmarshalling", []() {
            Option originalOption;
            originalOption.mode = 0;
            originalOption.seqNum = 12345;
            originalOption.isAsync = true;
            originalOption.isAutoSync = true;
            originalOption.isCompensation = false;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalOption)) {
                return false;
            }

            Option unmarshalledOption;
            return ITypesUtil::Unmarshal(parcel, unmarshalledOption) &&
                   unmarshalledOption.seqNum == originalOption.seqNum &&
                   unmarshalledOption.isAsync == originalOption.isAsync;
        }},
        {"SubOption::Unmarshalling", []() {
            SubOption originalOption;
            originalOption.mode = SubscribeMode::CLOUD;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalOption)) {
                return false;
            }

            SubOption unmarshalledOption;
            return ITypesUtil::Unmarshal(parcel, unmarshalledOption) &&
                   unmarshalledOption.mode == originalOption.mode;
        }},
        {"RdbChangedData::Unmarshalling", []() {
            RdbChangedData originalChangedData;
            RdbChangeProperties changeProps;
            changeProps.isTrackedDataChange = true;
            changeProps.isP2pSyncDataChange = false;
            originalChangedData.tableData["table"] = changeProps;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalChangedData)) {
                return false;
            }

            RdbChangedData unmarshalledChangedData;
            return ITypesUtil::Unmarshal(parcel, unmarshalledChangedData) &&
                   unmarshalledChangedData.tableData.size() == originalChangedData.tableData.size();
        }},
        {"RdbProperties::Unmarshalling", []() {
            RdbProperties originalProperties;
            originalProperties.isTrackedDataChange = true;
            originalProperties.isP2pSyncDataChange = false;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalProperties)) {
                return false;
            }

            RdbProperties unmarshalledProperties;
            return ITypesUtil::Unmarshal(parcel, unmarshalledProperties) &&
                   unmarshalledProperties.isTrackedDataChange == originalProperties.isTrackedDataChange;
        }},
        {"Reference::Unmarshalling", []() {
            Reference originalRef;
            originalRef.sourceTable = "source";
            originalRef.targetTable = "target";
            originalRef.refFields.insert({"f1", "f1_value"});

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalRef)) {
                return false;
            }

            Reference unmarshalledRef;
            return ITypesUtil::Unmarshal(parcel, unmarshalledRef) &&
                   unmarshalledRef.sourceTable == originalRef.sourceTable &&
                   unmarshalledRef.refFields.size() == originalRef.refFields.size();
        }},
        {"StatReporter::Unmarshalling", []() {
            StatReporter originalReporter;
            originalReporter.statType = 1;
            originalReporter.bundleName = "com.example.app";
            originalReporter.storeName = "test.db";
            originalReporter.subType = 0;
            originalReporter.costTime = 100;

            MessageParcel parcel;
            if (!ITypesUtil::Marshal(parcel, originalReporter)) {
                return false;
            }

            StatReporter unmarshalledReporter;
            return ITypesUtil::Unmarshal(parcel, unmarshalledReporter) &&
                   unmarshalledReporter.bundleName == originalReporter.bundleName &&
                   unmarshalledReporter.costTime == originalReporter.costTime;
        }}
    };

    for (const auto& [name, testFunc] : tests) {
        EXPECT_TRUE(testFunc()) << "Test failed for: " << name;
    }
}

} // namespace Test
} // namespace OHOS
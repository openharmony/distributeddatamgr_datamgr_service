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

#include "rdb_types_utils.h"

#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "itypes_util.h"
#include "message_parcel.h"
#include "parcel.h"

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

static NotifyConfig CreateTestNotifyConfig(uint32_t delay = 1000, bool isFull = true)
{
    NotifyConfig config;
    config.delay_ = delay;
    config.isFull_ = isFull;
    return config;
}

static Option CreateTestOption(int32_t mode = 0, int32_t seqNum = 12345, bool isAsync = true, bool isAutoSync = false,
    bool isCompensation = false)
{
    Option option;
    option.mode = mode;
    option.seqNum = seqNum;
    option.isAsync = isAsync;
    option.isAutoSync = isAutoSync;
    option.isCompensation = isCompensation;
    return option;
}

static SubOption CreateTestSubOption(SubscribeMode mode = SubscribeMode::CLOUD)
{
    SubOption option;
    option.mode = mode;
    return option;
}

static RdbChangedData CreateTestChangedData(
    const std::string &tableName = "table", bool isTracked = true, bool isP2p = false)
{
    RdbChangedData data;
    RdbChangeProperties props;
    props.isTrackedDataChange = isTracked;
    props.isP2pSyncDataChange = isP2p;
    data.tableData[tableName] = props;
    return data;
}

static RdbProperties CreateTestProperties(bool isTracked = true, bool isP2p = false)
{
    RdbProperties props;
    props.isTrackedDataChange = isTracked;
    props.isP2pSyncDataChange = isP2p;
    return props;
}

static Reference CreateTestReference(const std::string &source = "source_table",
    const std::string &target = "target_table", const std::vector<std::pair<std::string, std::string>> &fields = {})
{
    Reference ref;
    ref.sourceTable = source;
    ref.targetTable = target;
    if (fields.empty()) {
        ref.refFields.insert({ "field1", "field1_value" });
        ref.refFields.insert({ "field2", "field2_value" });
        ref.refFields.insert({ "field3", "field3_value" });
    } else {
        for (const auto &[key, value] : fields) {
            ref.refFields.insert({ key, value });
        }
    }
    return ref;
}

static StatReporter CreateTestStatReporter(int32_t statType = 1, const std::string &bundleName = "com.example.app",
    const std::string &storeName = "test.db", int32_t subType = 0, int32_t costTime = 100)
{
    StatReporter reporter;
    reporter.statType = statType;
    reporter.bundleName = bundleName;
    reporter.storeName = storeName;
    reporter.subType = subType;
    reporter.costTime = costTime;
    return reporter;
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_NotifyConfig_Basic
 * @tc.desc: Test basic unmarshalling functionality for NotifyConfig
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_NotifyConfig_Basic, TestSize.Level1)
{
    auto originalConfig = CreateTestNotifyConfig();

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

    NotifyConfig unmarshalledConfig;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledConfig));
    EXPECT_EQ(unmarshalledConfig.delay_, originalConfig.delay_);
    EXPECT_EQ(unmarshalledConfig.isFull_, originalConfig.isFull_);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_NotifyConfig_DifferentValues
 * @tc.desc: Test unmarshalling for NotifyConfig with different parameter values
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_NotifyConfig_DifferentValues, TestSize.Level1)
{
    struct TestCase {
        uint32_t delay;
        bool isFull;
    };

    std::vector<TestCase> testCases = { { 0, false }, { 100, false }, { 1000, true }, { UINT32_MAX, false },
        { 5000, true } };

    for (const auto &testCase : testCases) {
        auto originalConfig = CreateTestNotifyConfig(testCase.delay, testCase.isFull);

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

        NotifyConfig unmarshalledConfig;
        EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledConfig))
            << "Failed for delay=" << testCase.delay << ", isFull=" << testCase.isFull;
        EXPECT_EQ(unmarshalledConfig.delay_, testCase.delay);
        EXPECT_EQ(unmarshalledConfig.isFull_, testCase.isFull);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Option_Complete
 * @tc.desc: Test complete unmarshalling for Option
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Option_Complete, TestSize.Level1)
{
    auto originalOption = CreateTestOption(1, 12345, true, false, true);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

    Option unmarshalledOption;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption));
    EXPECT_EQ(unmarshalledOption.mode, originalOption.mode);
    EXPECT_EQ(unmarshalledOption.seqNum, originalOption.seqNum);
    EXPECT_EQ(unmarshalledOption.isAsync, originalOption.isAsync);
    EXPECT_EQ(unmarshalledOption.isAutoSync, originalOption.isAutoSync);
    EXPECT_EQ(unmarshalledOption.isCompensation, originalOption.isCompensation);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Option_FlagsCombinations
 * @tc.desc: Test unmarshalling for Option with all flag combinations
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Option_FlagsCombinations, TestSize.Level1)
{
    for (int isAsync = 0; isAsync <= 1; isAsync++) {
        for (int isAutoSync = 0; isAutoSync <= 1; isAutoSync++) {
            for (int isCompensation = 0; isCompensation <= 1; isCompensation++) {
                auto originalOption = CreateTestOption(0, 100, isAsync, isAutoSync, isCompensation);

                MessageParcel parcel;
                ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

                Option unmarshalledOption;
                EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption));
                EXPECT_EQ(unmarshalledOption.mode, originalOption.mode);
                EXPECT_EQ(unmarshalledOption.seqNum, originalOption.seqNum);
                EXPECT_EQ(unmarshalledOption.isAsync, static_cast<bool>(isAsync));
                EXPECT_EQ(unmarshalledOption.isAutoSync, static_cast<bool>(isAutoSync));
                EXPECT_EQ(unmarshalledOption.isCompensation, static_cast<bool>(isCompensation));
            }
        }
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_SubOption_AllModes
 * @tc.desc: Test unmarshalling for SubOption with all subscription modes
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_SubOption_AllModes, TestSize.Level1)
{
    std::vector<SubscribeMode> modes = { SubscribeMode::CLOUD, SubscribeMode::LOCAL, SubscribeMode::REMOTE };

    for (auto mode : modes) {
        auto originalOption = CreateTestSubOption(mode);

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

        SubOption unmarshalledOption;
        EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption))
            << "Failed for SubscribeMode: " << static_cast<int>(mode);
        EXPECT_EQ(unmarshalledOption.mode, mode);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_SubOption_ModeValue
 * @tc.desc: Test unmarshalling for SubOption with mode value conversion
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_SubOption_ModeValue, TestSize.Level1)
{
    std::vector<int32_t> modeValues = { 0, 1, 2, 10, 100, -1 };

    for (int32_t modeValue : modeValues) {
        SubOption originalOption;
        originalOption.mode = static_cast<SubscribeMode>(modeValue);

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

        SubOption unmarshalledOption;
        EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption)) << "Failed for mode value: " << modeValue;
        EXPECT_EQ(static_cast<int32_t>(unmarshalledOption.mode), modeValue);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_SingleTable
 * @tc.desc: Test unmarshalling for single table RdbChangedData
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_SingleTable, TestSize.Level1)
{
    auto originalChangedData = CreateTestChangedData("test_table", true, false);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    RdbChangedData unmarshalledChangedData;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledChangedData));
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());
    EXPECT_EQ(unmarshalledChangedData.tableData["test_table"].isTrackedDataChange,
        originalChangedData.tableData["test_table"].isTrackedDataChange);
    EXPECT_EQ(unmarshalledChangedData.tableData["test_table"].isP2pSyncDataChange,
        originalChangedData.tableData["test_table"].isP2pSyncDataChange);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_MultipleTables
 * @tc.desc: Test unmarshalling for multiple tables RdbChangedData
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_MultipleTables, TestSize.Level1)
{
    RdbChangedData originalChangedData;

    for (int i = 0; i < 5; i++) {
        std::string tableName = "table_" + std::to_string(i);
        RdbChangeProperties properties;
        properties.isTrackedDataChange = (i % 2 == 0);
        properties.isP2pSyncDataChange = (i % 3 == 0);
        originalChangedData.tableData[tableName] = properties;
    }

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    RdbChangedData unmarshalledChangedData;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledChangedData));
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());

    for (const auto &[tableName, properties] : originalChangedData.tableData) {
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isTrackedDataChange, properties.isTrackedDataChange);
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isP2pSyncDataChange, properties.isP2pSyncDataChange);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbChangedData_EmptyTableData
 * @tc.desc: Test unmarshalling for empty tableData
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbChangedData_EmptyTableData, TestSize.Level1)
{
    RdbChangedData originalChangedData;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));

    RdbChangedData unmarshalledChangedData;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledChangedData));
    EXPECT_EQ(unmarshalledChangedData.tableData.size(), 0);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_RdbProperties_PropertyCombinations
 * @tc.desc: Test unmarshalling for RdbProperties with property combinations
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_RdbProperties_PropertyCombinations, TestSize.Level1)
{
    struct TestCase {
        bool isTrackedDataChange;
        bool isP2pSyncDataChange;
    };

    std::vector<TestCase> testCases = { { false, false }, { true, false }, { false, true }, { true, true } };

    for (const auto &testCase : testCases) {
        auto originalProperties = CreateTestProperties(testCase.isTrackedDataChange, testCase.isP2pSyncDataChange);

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalProperties));

        RdbProperties unmarshalledProperties;
        EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledProperties))
            << "Failed for tracked=" << testCase.isTrackedDataChange << ", p2p=" << testCase.isP2pSyncDataChange;
        EXPECT_EQ(unmarshalledProperties.isTrackedDataChange, testCase.isTrackedDataChange);
        EXPECT_EQ(unmarshalledProperties.isP2pSyncDataChange, testCase.isP2pSyncDataChange);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Reference_Basic
 * @tc.desc: Test basic unmarshalling for Reference
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Reference_Basic, TestSize.Level1)
{
    auto originalRef = CreateTestReference();

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));

    Reference unmarshalledRef;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledRef));
    EXPECT_EQ(unmarshalledRef.sourceTable, originalRef.sourceTable);
    EXPECT_EQ(unmarshalledRef.targetTable, originalRef.targetTable);
    EXPECT_EQ(unmarshalledRef.refFields.size(), originalRef.refFields.size());
    EXPECT_EQ(unmarshalledRef.refFields["field1"], "field1_value");
    EXPECT_EQ(unmarshalledRef.refFields["field2"], "field2_value");
    EXPECT_EQ(unmarshalledRef.refFields["field3"], "field3_value");
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Reference_MultipleFields
 * @tc.desc: Test unmarshalling for Reference with multiple fields
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Reference_MultipleFields, TestSize.Level1)
{
    Reference originalRef;
    originalRef.sourceTable = "src";
    originalRef.targetTable = "tgt";

    std::vector<size_t> fieldCounts = { 0, 1, 5, 10 };

    for (size_t fieldCount : fieldCounts) {
        originalRef.refFields.clear();
        for (size_t i = 0; i < fieldCount; i++) {
            std::string fieldName = "field_" + std::to_string(i);
            originalRef.refFields.insert({ fieldName, fieldName + "_value" });
        }

        MessageParcel parcel;
        ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));

        Reference unmarshalledRef;
        EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledRef)) << "Failed with " << fieldCount << " fields";
        EXPECT_EQ(unmarshalledRef.sourceTable, originalRef.sourceTable);
        EXPECT_EQ(unmarshalledRef.targetTable, originalRef.targetTable);
        EXPECT_EQ(unmarshalledRef.refFields.size(), originalRef.refFields.size());
        for (const auto &[key, value] : originalRef.refFields) {
            EXPECT_EQ(unmarshalledRef.refFields[key], value);
        }
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_StatReporter_Complete
 * @tc.desc: Test complete unmarshalling for StatReporter
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_StatReporter_Complete, TestSize.Level1)
{
    auto originalReporter = CreateTestStatReporter(3, "com.example.app", "test.db", 1, 250);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    StatReporter unmarshalledReporter;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReporter));
    EXPECT_EQ(unmarshalledReporter.statType, originalReporter.statType);
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
    EXPECT_EQ(unmarshalledReporter.storeName, originalReporter.storeName);
    EXPECT_EQ(unmarshalledReporter.subType, originalReporter.subType);
    EXPECT_EQ(unmarshalledReporter.costTime, originalReporter.costTime);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_StatReporter_LongStrings
 * @tc.desc: Test unmarshalling for long strings
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_StatReporter_LongStrings, TestSize.Level1)
{
    auto originalReporter = CreateTestStatReporter(1, std::string(1000, 'a'), std::string(1000, 'b'), 0, 999999);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    StatReporter unmarshalledReporter;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReporter));
    EXPECT_EQ(unmarshalledReporter.statType, originalReporter.statType);
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
    EXPECT_EQ(unmarshalledReporter.storeName, originalReporter.storeName);
    EXPECT_EQ(unmarshalledReporter.subType, originalReporter.subType);
    EXPECT_EQ(unmarshalledReporter.costTime, originalReporter.costTime);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_MaxDelayValue
 * @tc.desc: Test unmarshalling for maximum delay value
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_MaxDelayValue, TestSize.Level1)
{
    auto originalConfig = CreateTestNotifyConfig(UINT32_MAX, true);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));

    NotifyConfig unmarshalledConfig;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledConfig));
    EXPECT_EQ(unmarshalledConfig.delay_, UINT32_MAX);
    EXPECT_EQ(unmarshalledConfig.isFull_, true);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_MaxSeqNum
 * @tc.desc: Test unmarshalling for maximum sequence number
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_MaxSeqNum, TestSize.Level1)
{
    auto originalOption = CreateTestOption(0, INT32_MAX, true, true, true);

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));

    Option unmarshalledOption;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption));
    EXPECT_EQ(unmarshalledOption.mode, originalOption.mode);
    EXPECT_EQ(unmarshalledOption.seqNum, INT32_MAX);
    EXPECT_EQ(unmarshalledOption.isAsync, true);
    EXPECT_EQ(unmarshalledOption.isAutoSync, true);
    EXPECT_EQ(unmarshalledOption.isCompensation, true);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Boundary_EmptyRefFields
 * @tc.desc: Test unmarshalling for empty reference fields
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Boundary_EmptyRefFields, TestSize.Level1)
{
    Reference originalRef;
    originalRef.sourceTable = "src";
    originalRef.targetTable = "tgt";
    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalRef));
    Reference unmarshalledRef;
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledRef));
    EXPECT_EQ(unmarshalledRef.sourceTable, "src");
    EXPECT_EQ(unmarshalledRef.targetTable, "tgt");
    EXPECT_EQ(unmarshalledRef.refFields.size(), 0);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Integration_TypesInOneParcel_Primitives
 * @tc.desc: Test marshalling and unmarshalling of primitive types in a single Parcel
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Integration_TypesInOneParcel_Primitives, TestSize.Level1)
{
    MessageParcel parcel;
    auto originalConfig = CreateTestNotifyConfig();
    auto originalOption = CreateTestOption();
    auto originalSubOption = CreateTestSubOption();

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalConfig));
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalOption));
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalSubOption));

    NotifyConfig unmarshalledConfig;
    Option unmarshalledOption;
    SubOption unmarshalledSubOption;

    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledConfig));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledOption));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledSubOption));

    EXPECT_EQ(unmarshalledConfig.delay_, originalConfig.delay_);
    EXPECT_EQ(unmarshalledConfig.isFull_, originalConfig.isFull_);
    EXPECT_EQ(unmarshalledOption.mode, originalOption.mode);
    EXPECT_EQ(unmarshalledOption.seqNum, originalOption.seqNum);
    EXPECT_EQ(unmarshalledOption.isAsync, originalOption.isAsync);
    EXPECT_EQ(unmarshalledOption.isAutoSync, originalOption.isAutoSync);
    EXPECT_EQ(unmarshalledOption.isCompensation, originalOption.isCompensation);
    EXPECT_EQ(unmarshalledSubOption.mode, originalSubOption.mode);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Integration_TypesInOneParcel_Complex
 * @tc.desc: Test marshalling and unmarshalling of complex types in a single Parcel
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Integration_TypesInOneParcel_Complex, TestSize.Level1)
{
    MessageParcel parcel;
    auto originalChangedData = CreateTestChangedData();
    auto originalProperties = CreateTestProperties();
    auto originalReference = CreateTestReference();
    auto originalReporter = CreateTestStatReporter();

    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalChangedData));
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalProperties));
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReference));
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, originalReporter));

    RdbChangedData unmarshalledChangedData;
    RdbProperties unmarshalledProperties;
    Reference unmarshalledReference;
    StatReporter unmarshalledReporter;

    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledChangedData));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledProperties));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReference));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalledReporter));

    EXPECT_EQ(unmarshalledChangedData.tableData.size(), originalChangedData.tableData.size());
    for (const auto &[tableName, properties] : originalChangedData.tableData) {
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isTrackedDataChange,
            properties.isTrackedDataChange);
        EXPECT_EQ(unmarshalledChangedData.tableData[tableName].isP2pSyncDataChange,
            properties.isP2pSyncDataChange);
    }
    EXPECT_EQ(unmarshalledProperties.isTrackedDataChange, originalProperties.isTrackedDataChange);
    EXPECT_EQ(unmarshalledProperties.isP2pSyncDataChange, originalProperties.isP2pSyncDataChange);
    EXPECT_EQ(unmarshalledReference.sourceTable, originalReference.sourceTable);
    EXPECT_EQ(unmarshalledReference.targetTable, originalReference.targetTable);
    EXPECT_EQ(unmarshalledReference.refFields.size(), originalReference.refFields.size());
    for (const auto &[key, value] : originalReference.refFields) {
        EXPECT_EQ(unmarshalledReference.refFields[key], value);
    }
    EXPECT_EQ(unmarshalledReporter.statType, originalReporter.statType);
    EXPECT_EQ(unmarshalledReporter.bundleName, originalReporter.bundleName);
    EXPECT_EQ(unmarshalledReporter.storeName, originalReporter.storeName);
    EXPECT_EQ(unmarshalledReporter.subType, originalReporter.subType);
    EXPECT_EQ(unmarshalledReporter.costTime, originalReporter.costTime);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_NotifyConfig
 * @tc.desc: Regression test for NotifyConfig interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_NotifyConfig, TestSize.Level1)
{
    auto config = CreateTestNotifyConfig();
    NotifyConfig unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, config));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.delay_, config.delay_);
    EXPECT_EQ(unmarshalled.isFull_, config.isFull_);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_Option
 * @tc.desc: Regression test for Option interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_Option, TestSize.Level1)
{
    auto option = CreateTestOption();
    Option unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, option));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.mode, option.mode);
    EXPECT_EQ(unmarshalled.seqNum, option.seqNum);
    EXPECT_EQ(unmarshalled.isAsync, option.isAsync);
    EXPECT_EQ(unmarshalled.isAutoSync, option.isAutoSync);
    EXPECT_EQ(unmarshalled.isCompensation, option.isCompensation);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_SubOption
 * @tc.desc: Regression test for SubOption interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_SubOption, TestSize.Level1)
{
    auto option = CreateTestSubOption();
    SubOption unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, option));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.mode, option.mode);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_RdbChangedData
 * @tc.desc: Regression test for RdbChangedData interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_RdbChangedData, TestSize.Level1)
{
    auto data = CreateTestChangedData();
    RdbChangedData unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, data));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.tableData.size(), data.tableData.size());
    for (const auto &[tableName, properties] : data.tableData) {
        EXPECT_EQ(unmarshalled.tableData[tableName].isTrackedDataChange, properties.isTrackedDataChange);
        EXPECT_EQ(unmarshalled.tableData[tableName].isP2pSyncDataChange, properties.isP2pSyncDataChange);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_RdbProperties
 * @tc.desc: Regression test for RdbProperties interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_RdbProperties, TestSize.Level1)
{
    auto props = CreateTestProperties();
    RdbProperties unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, props));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.isTrackedDataChange, props.isTrackedDataChange);
    EXPECT_EQ(unmarshalled.isP2pSyncDataChange, props.isP2pSyncDataChange);
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_Reference
 * @tc.desc: Regression test for Reference interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_Reference, TestSize.Level1)
{
    auto ref = CreateTestReference();
    Reference unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, ref));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.sourceTable, ref.sourceTable);
    EXPECT_EQ(unmarshalled.targetTable, ref.targetTable);
    EXPECT_EQ(unmarshalled.refFields.size(), ref.refFields.size());
    for (const auto &[key, value] : ref.refFields) {
        EXPECT_EQ(unmarshalled.refFields[key], value);
    }
}

/**
 * @tc.name: RdbTypesUtil_Unmarshal_Regression_StatReporter
 * @tc.desc: Regression test for StatReporter interface
 * @tc.type: FUNC
 */
HWTEST_F(RdbTypesUtilsTest, RdbTypesUtil_Unmarshal_Regression_StatReporter, TestSize.Level1)
{
    auto reporter = CreateTestStatReporter();
    StatReporter unmarshalled;

    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, reporter));
    EXPECT_TRUE(ITypesUtil::Unmarshal(parcel, unmarshalled));
    EXPECT_EQ(unmarshalled.statType, reporter.statType);
    EXPECT_EQ(unmarshalled.bundleName, reporter.bundleName);
    EXPECT_EQ(unmarshalled.storeName, reporter.storeName);
    EXPECT_EQ(unmarshalled.subType, reporter.subType);
    EXPECT_EQ(unmarshalled.costTime, reporter.costTime);
}

} // namespace Test
} // namespace OHOS

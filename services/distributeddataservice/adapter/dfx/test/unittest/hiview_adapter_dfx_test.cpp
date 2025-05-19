/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "HiViewAdapterDfxTest"

#include "device_manager_adapter.h"
#include "dfx/dfx_types.h"
#include "dfx/radar_reporter.h"
#include "gtest/gtest.h"
#include "hiview_adapter.h"
#include "log_print.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class HiViewAdapterDfxTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class RadarReporterDfxTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: ReportFault001
* @tc.desc: ReportFault function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportFault001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::FaultMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportFault(dfxCode, msg, executors));
}

/**
* @tc.name: ReportDBFault001
* @tc.desc: ReportDBFault function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportDBFault001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::DBFaultMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportDBFault(dfxCode, msg, executors));
}

/**
* @tc.name: ReportCommFault001
* @tc.desc: ReportCommFault function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportCommFault001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::CommFaultMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportCommFault(dfxCode, msg, executors));
}

/**
* @tc.name: ReportArkDataFault001
* @tc.desc: ReportArkDataFault function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportArkDataFault001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::ArkDataFaultMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportArkDataFault(dfxCode, msg, executors));
}

/**
* @tc.name: ReportBehaviour001
* @tc.desc: ReportBehaviour function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportBehaviour001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::BehaviourMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportBehaviour(dfxCode, msg, executors));
}

/**
* @tc.name: ReportDatabaseStatistic001
* @tc.desc: ReportDatabaseStatistic function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportDatabaseStatistic001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::DbStat stat;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportDatabaseStatistic(dfxCode, stat, executors));
}

/**
* @tc.name: ReportTrafficStatistic001
* @tc.desc: ReportTrafficStatistic function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportTrafficStatistic001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::TrafficStat stat;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportTrafficStatistic(dfxCode, stat, executors));
}

/**
* @tc.name: ReportVisitStatistic001
* @tc.desc: ReportVisitStatistic function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportVisitStatistic001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::VisitStat stat;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportVisitStatistic(dfxCode, stat, executors));
}

/**
* @tc.name: ReportApiPerformanceStatistic001
* @tc.desc: ReportApiPerformanceStatistic function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportApiPerformanceStatistic001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::ApiPerformanceStat stat;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportApiPerformanceStatistic(dfxCode, stat, executors));
}

/**
* @tc.name: ReportUdmfBehaviour001
* @tc.desc: ReportUdmfBehaviour function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, ReportUdmfBehaviour001, TestSize.Level0)
{
    int dfxCode = 0;
    DistributedDataDfx::UdmfBehaviourMsg msg;
    std::shared_ptr<ExecutorPool> executors = nullptr;
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::HiViewAdapter::ReportUdmfBehaviour(dfxCode, msg, executors));
}

/**
* @tc.name: CoverEventID001
* @tc.desc: CoverEventID function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(HiViewAdapterDfxTest, CoverEventID001, TestSize.Level0)
{
    int dfxCode = 0;
    std::string str = DistributedDataDfx::HiViewAdapter::CoverEventID(dfxCode);
    EXPECT_EQ(str, "");
}

/**
* @tc.name: AnonymousUuid001
* @tc.desc: AnonymousUuid function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RadarReporterDfxTest, AnonymousUuid001, TestSize.Level0)
{
    std::string uuid = "uuid";
    std::string str = DistributedDataDfx::RadarReporter::AnonymousUuid(uuid);
    EXPECT_EQ(str, DistributedDataDfx::RadarReporter::DEFAULT_ANONYMOUS);
}

/**
* @tc.name: Report001
* @tc.desc: Report function error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RadarReporterDfxTest, Report001, TestSize.Level0)
{
    DistributedDataDfx::RadarParam param;
    const char *funcName = "funcName";
    int32_t state = 0;
    const char *eventName = "eventName";
    EXPECT_NO_FATAL_FAILURE(
        DistributedDataDfx::RadarReporter::Report(param, funcName, state, eventName));
}
} // namespace OHOS::Test
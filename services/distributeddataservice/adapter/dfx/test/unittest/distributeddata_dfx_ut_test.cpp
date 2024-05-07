/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "reporter.h"
#include "fake_hiview.h"
#include "value_hash.h"

using namespace testing::ext;
using namespace OHOS::DistributedDataDfx;

class DistributedataDfxUTTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
};

void DistributedataDfxUTTest::SetUpTestCase()
{
    FakeHivew::Clear();
}

void DistributedataDfxUTTest::TearDownTestCase()
{
    FakeHivew::Clear();
}

void DistributedataDfxUTTest::SetUp()
{
    size_t max = 12;
    size_t min = 5;
    Reporter::GetInstance()->SetThreadPool(std::make_shared<OHOS::ExecutorPool>(max, min));
}

void DistributedataDfxUTTest::TearDown()
{
    Reporter::GetInstance()->SetThreadPool(nullptr);
}

/**
  * @tc.name: Dfx001
  * @tc.desc: send data to 1 device, then check reporter message.
  * @tc.type: send data
  * @tc.require: AR000CQE1L SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. getcommunicationFault instance
     * @tc.expected: step1. Expect instance is not null.
     */
    auto comFault = Reporter::GetInstance()->CommunicationFault();
    EXPECT_NE(nullptr, comFault);
    struct CommFaultMsg msg{.userId = "user001", .appId = "myApp", .storeId = "storeTest"};
    msg.deviceId.push_back("device001");
    msg.errorCode.push_back(001);
    msg.deviceId.push_back("device002");
    msg.errorCode.push_back(002);

    auto repStatus = comFault->Report(msg);
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    /**
    * @tc.steps:step2. check dfx reporter.
    * @tc.expected: step2. Expect report message success.
    */
    std::string val = FakeHivew::GetString("ANONYMOUS_UID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("user001").c_str());
    }
    val = FakeHivew::GetString("APP_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("myApp").c_str());
    }
    val = FakeHivew::GetString("STORE_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("storeTest").c_str());
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx002
  * @tc.desc: check database dfx report
  * @tc.type: get kvStore
  * @tc.require: AR000CQE1L AR000CQE1K SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx002, TestSize.Level0)
{
    FakeHivew::Clear();
    /**
     * @tc.steps: step1. get database fault report instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto dbFault = Reporter::GetInstance()->DatabaseFault();
    EXPECT_NE(nullptr, dbFault);
    struct DBFaultMsg msg {.appId = "MyApp", .storeId = "MyDatabase",
        .moduleName = "database", .errorType = Fault::DF_DB_DAMAGE};

    auto repStatus = dbFault->Report(msg);
    /**
     * @tc.steps: step2. check fault reporter.
     * @tc.expected: step2. Expect report has bad report message.
     */
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    auto val = FakeHivew::GetString("MODULE_NAME");
    if (!val.empty()) {
        EXPECT_STREQ(string("database").c_str(), val.c_str());
    }
    auto typeVal = FakeHivew::GetInt("ERROR_TYPE");
    if (typeVal > 0) {
        EXPECT_EQ(static_cast<int>(Fault::DF_DB_DAMAGE), typeVal);
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx003
  * @tc.desc: Database file size statistic.
  * @tc.type: check database file size.
  * @tc.require: AR000CQE1O SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. get database reporter instance.
     * @tc.expected: step1. Expect get success.
     */
    auto dbs = Reporter::GetInstance()->DatabaseStatistic();
    EXPECT_NE(nullptr, dbs);
    DbStat ds = {"uid", "appid", "storeId001", 100};
    auto dbsRet = dbs->Report(ds);
    /**
     * @tc.steps:step2. check reporter.
     * @tc.expected: step2. Expect statistic database size is 100.
     */
    EXPECT_TRUE(dbsRet == ReportStatus::SUCCESS);
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx004
  * @tc.desc: Set invalid information, call getKvStore, expect return INVALID_ARGS.
  * @tc.type: CreateKvStore test
  * @tc.require: AR000CQE1L SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Get runtime fault instance.
     * @tc.expected: step1. Expect get runtime fault instance success.
     */
    auto rtFault = Reporter::GetInstance()->RuntimeFault();
    auto rtFault2 = Reporter::GetInstance()->RuntimeFault();
    EXPECT_NE(nullptr, rtFault);
    EXPECT_EQ(rtFault, rtFault2);

    struct FaultMsg rfMsg{FaultType::SERVICE_FAULT, "database", "closeKvStore",
                          Fault::RF_CLOSE_DB};
    auto rfReportRet = rtFault->Report(rfMsg);
    /**
     * @tc.steps:step2. check report message.
     * @tc.expected: step2. Expected reported message.
     */
    EXPECT_TRUE(rfReportRet == ReportStatus::SUCCESS);
    auto val = FakeHivew::GetString("INTERFACE_NAME");
    if (!val.empty()) {
        EXPECT_STREQ(string("closeKvStore").c_str(), val.c_str());
    }
    auto typeVal = FakeHivew::GetInt("ERROR_TYPE");
    if (typeVal > 0) {
        EXPECT_EQ(static_cast<int>(Fault::RF_CLOSE_DB), typeVal);
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx005
  * @tc.desc: send data to 1 device, then check send size.
  * @tc.type: send data
  * @tc.require: AR000CQE1P SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. send data to 1 device
     * @tc.expected: step1. Expect put success.
     */
    auto ts = Reporter::GetInstance()->TrafficStatistic();
    EXPECT_NE(nullptr, ts);
    struct TrafficStat tss = {"appId001", "deviceId001", 100, 200};
    auto tsRet = ts->Report(tss);
    /**
     * @tc.steps:step2. check dfx reporter.
     * @tc.expected: step2. Expect report has same size.
     */
    EXPECT_TRUE(tsRet == ReportStatus::SUCCESS);

    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx006
  * @tc.desc: call interface statistic.
  * @tc.type: statistic
  * @tc.require: AR000CQE1N SR000CQE1J
  * @tc.author: hongbo
  */
HWTEST_F(DistributedataDfxUTTest, Dfx006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. create call interface statistic instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto vs = Reporter::GetInstance()->VisitStatistic();
    EXPECT_NE(nullptr, vs);
    struct VisitStat vss = {"appid001", "Put"};
    auto vsRet = vs->Report(vss);
    /**
     * @tc.steps:step2. check dfx reporter.
     * @tc.expected: step2. Expect report has same information.
     */
    EXPECT_TRUE(vsRet == ReportStatus::SUCCESS);

    FakeHivew::Clear();
    std::string myuid = "203230afadj020003";
    std::string result;
    ValueHash vh;
    vh.CalcValueHash(myuid, result);
}

/**
  * @tc.name: Dfx007
  * @tc.desc: call api performance statistic.
  * @tc.type: statistic
  * @tc.require: AR000DPVGP SR000DPVGH
  * @tc.author: liwei
  */
HWTEST_F(DistributedataDfxUTTest, Dfx007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. create call api perforamnce statistic instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto ap = Reporter::GetInstance()->ApiPerformanceStatistic();
    EXPECT_NE(nullptr, ap);
    struct ApiPerformanceStat aps = { "interface", 10000, 5000, 20000 };
    auto apRet = ap->Report(aps);
    /**
     * @tc.steps:step2. check dfx reporter return value.
     * @tc.expected: step2. Expect report has same information.
     */
    EXPECT_TRUE(apRet == ReportStatus::SUCCESS);

    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx008
  * @tc.desc: send data to 1 device, then check reporter message.
  * @tc.type: send data
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx008, TestSize.Level0)
{
     /**
     * @tc.steps: step1. get database fault report instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto behavior = Reporter::GetInstance()->BehaviourReporter();
    EXPECT_NE(nullptr, behavior);
    struct BehaviourMsg msg{.userId = "user008", .appId = "myApp08", .storeId = "storeTest08",
                            .behaviourType = BehaviourType::DATABASE_BACKUP, .extensionInfo="test111"};

    auto repStatus = behavior->Report(msg);
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    /**
    * @tc.steps:step2. check dfx reporter.
    * @tc.expected: step2. Expect report message success.
    */
    std::string val = FakeHivew::GetString("ANONYMOUS_UID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("user008").c_str());
    }
    val = FakeHivew::GetString("APP_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("myApp08").c_str());
    }
    val = FakeHivew::GetString("STORE_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("storeTest08").c_str());
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx009
  * @tc.desc: Set invalid information, call getKvStore, expect return INVALID_ARGS.
  * @tc.type: CreateKvStore test
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Get runtime fault instance.
     * @tc.expected: step1. Expect get runtime fault instance success.
     */
    auto svFault = Reporter::GetInstance()->ServiceFault();
    auto svFault2 = Reporter::GetInstance()->ServiceFault();
    EXPECT_NE(nullptr, svFault);
    EXPECT_EQ(svFault, svFault2);

    struct FaultMsg svMsg{FaultType::SERVICE_FAULT, "myData", "createKvStore",
                          Fault::SF_CREATE_DIR};
    auto rfReportRet = svFault->Report(svMsg);
    /**
     * @tc.steps:step2. check report message.
     * @tc.expected: step2. Expected reported message.
     */
    EXPECT_TRUE(rfReportRet == ReportStatus::SUCCESS);
    auto val = FakeHivew::GetString("INTERFACE_NAME");
    if (!val.empty()) {
        EXPECT_STREQ(string("createKvStore").c_str(), val.c_str());
    }
    auto typeVal = FakeHivew::GetInt("ERROR_TYPE");
    if (typeVal > 0) {
        EXPECT_EQ(static_cast<int>(Fault::SF_CREATE_DIR), typeVal);
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx010
  * @tc.desc: Database file size statistic.
  * @tc.type: check database file size.
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. get database reporter instance.
     * @tc.expected: step1. Expect get success.
     */
    auto dbs = Reporter::GetInstance()->DatabaseStatistic();
    EXPECT_NE(nullptr, dbs);
    DbStat ds = {"uid", "appid", "storeId002", 100};
    auto dbsRet = dbs->Report(ds);
    /**
     * @tc.steps:step2. check reporter.
     * @tc.expected: step2. Expect statistic database size is 100.
     */
    EXPECT_TRUE(dbsRet == ReportStatus::SUCCESS);
    auto val = FakeHivew::GetString("STORE_ID");
    if (!val.empty()) {
        EXPECT_STREQ(string("storeId002").c_str(), val.c_str());
    }
    auto typeVal = FakeHivew::GetInt("DB_SIZE");
    if (typeVal > 0) {
        EXPECT_EQ(100, typeVal);
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx011
  * @tc.desc: Send UdmfBehaviourMsg
  * @tc.type: Send data
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx011, TestSize.Level0)
{
     /**
     * @tc.steps: step1. get database fault report instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto UdmfBehavior = Reporter::GetInstance()->BehaviourReporter();
    EXPECT_NE(nullptr, UdmfBehavior);
    struct UdmfBehaviourMsg UdMsg{"myApp", "channel", 200, "dataType", "operation", "result"};
    auto repStatus = UdmfBehavior->UDMFReport(UdMsg);
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    /**
    * @tc.steps:step2. check dfx reporter.
    * @tc.expected: step2. Expect report message success.
    */
    auto val = FakeHivew::GetString("APP_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("myApp").c_str());
    }
    auto val01 = FakeHivew::GetString("CHANNEL");
    if (!val01.empty()) {
        EXPECT_STREQ(val01.c_str(), string("channel").c_str());
    }
    auto val02 = FakeHivew::GetString("OPERATION");
    if (!val02.empty()) {
        EXPECT_STREQ(val02.c_str(), string("operation").c_str());
    }
    auto val03 = FakeHivew::GetString("RESULT");
    if (!val03.empty()) {
        EXPECT_STREQ(val03.c_str(), string("result").c_str());
    }
    auto val04 = FakeHivew::GetInt("DB_SIZE");
    if (val04 > 0) {
        EXPECT_EQ(200, val04);
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx012
  * @tc.desc: send data to 1 device, then check send size.
  * @tc.type: send data
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx012, TestSize.Level0)
{
    /**
     * @tc.steps:step1. send data to 1 device
     * @tc.expected: step1. Expect put success.
     */
    auto ts = Reporter::GetInstance()->TrafficStatistic();
    EXPECT_NE(nullptr, ts);
    struct TrafficStat tss = {"appId001", "deviceId001", 100, 200};
    auto tsRet = ts->Report(tss);
    /**
     * @tc.steps:step2. check dfx reporter.
     * @tc.expected: step2. Expect report has same size.
     */
    EXPECT_TRUE(tsRet == ReportStatus::SUCCESS);
    std::string myuid = "cjsdblvdfbfss11";
    std::string result;
    ValueHash vh;
    vh.CalcValueHash(myuid, result);

    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx013
  * @tc.desc: call api performance statistic.
  * @tc.type:
  * @tc.author: nhj
  */
HWTEST_F(DistributedataDfxUTTest, Dfx013, TestSize.Level0)
{
    /**
     * @tc.steps:step1. create call api perforamnce statistic instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto ap = Reporter::GetInstance()->ApiPerformanceStatistic();
    EXPECT_NE(nullptr, ap);
    struct ApiPerformanceStat aps = { "interface", 2000, 500, 1000 };
    auto apRet = ap->Report(aps);
    /**
     * @tc.steps:step2. check dfx reporter return value.
     * @tc.expected: step2. Expect report has same information.
     */
    EXPECT_TRUE(apRet == ReportStatus::SUCCESS);
    auto val = FakeHivew::GetString("INTERFACE_NAME");
    if (!val.empty()) {
        EXPECT_STREQ(string("interface").c_str(), val.c_str());
    }
    FakeHivew::Clear();
}
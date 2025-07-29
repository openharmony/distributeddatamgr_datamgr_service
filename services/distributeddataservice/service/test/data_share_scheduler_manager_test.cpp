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
#define LOG_TAG "DataShareSchedulerManagerTest"

#include <gtest/gtest.h>

#include "log_print.h"
#include "db_delegate.h"
#include "template_data.h"
#include "scheduler_manager.h"

namespace OHOS::Test {
using namespace OHOS::DataShare;
using namespace testing::ext;
class DataShareSchedulerManagerTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: EnableScheduler001
* @tc.desc: test EnableScheduler function when not find key
* @tc.type: FUNC
* @tc.require:
* @tc.precon: None
* @tc.step:
    1.Add a template
    2.Creat a SchedulerManager object
    3.call Enable Scheduler function
* @tc.experct: not find key, Enable SchedulerStatus and GetSchedulerStatus return false
*/
HWTEST_F(DataShareSchedulerManagerTest, EnableScheduler001, TestSize.Level1)
{
    ZLOGI("EnableScheduler001 start");
    Key key("uri1", 12345, "name1");
    Template tplt;
    tplt.predicates_.emplace_back("test", "predicate test");
    tplt.scheduler_ = "scheduler test";
    std::string dataDir = "/data/service/el1/public/database/distributeddata/kvdb";
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 1);
    auto kvDelegate = KvDBDelegate::GetInstance(dataDir, executors);
    ASSERT_NE(kvDelegate, nullptr);
    TemplateData data(key.uri, key.bundleName, key.subscriberId, USER_TEST, tplt);
    auto ret = kvDelegate->Upsert(KvDBDelegate::TEMPLATE_TABLE, data);
    EXPECT_EQ(ret.first, OHOS::DataShare::E_OK);
    EXPECT_GT(ret.second, 0);

    auto& manager = SchedulerManager::GetInstance();
    DistributedData::StoreMetaData metaData;
    manager.Enable(key, USER_TEST, metaData);
    bool status = manager.GetSchedulerStatus(key);
    EXPECT_FALSE(status);

    ret = kvDelegate->Delete(KvDBDelegate::TEMPLATE_TABLE,
        static_cast<std::string>(Id(TemplateData::GenId(key.uri, key.bundleName, key.subscriberId), USER_TEST)));
    EXPECT_EQ(ret.first, OHOS::DataShare::E_OK);
    EXPECT_GT(ret.second, 0);
    ZLOGI("EnableScheduler001 end");
}

/**
* @tc.name: DisableScheduler001
* @tc.desc: test EnableScheduler function when not find key
* @tc.type: FUNC
* @tc.require:
* @tc.precon: None
* @tc.step:
    1.Add a template
    2.Creat a SchedulerManager object
    3.call Disable Scheduler function
* @tc.experct: not find key, Disable SchedulerStatus and GetSchedulerStatus return false
*/
HWTEST_F(DataShareSchedulerManagerTest, DisableScheduler001, TestSize.Level1)
{
    ZLOGI("DisableScheduler001 start");
    Key key("uri1", 12345, "name1");
    Template tplt;
    tplt.predicates_.emplace_back("test", "predicate test");
    tplt.scheduler_ = "scheduler test";
    std::string dataDir = "/data/service/el1/public/database/distributeddata/kvdb";
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 1);
    auto kvDelegate = KvDBDelegate::GetInstance(dataDir, executors);
    ASSERT_NE(kvDelegate, nullptr);
    TemplateData data(key.uri, key.bundleName, key.subscriberId, USER_TEST, tplt);
    auto ret = kvDelegate->Upsert(KvDBDelegate::TEMPLATE_TABLE, data);
    EXPECT_EQ(ret.first, OHOS::DataShare::E_OK);
    EXPECT_GT(ret.second, 0);

    auto& manager = SchedulerManager::GetInstance();
    manager.Disable(key);
    bool status = manager.GetSchedulerStatus(key);
    EXPECT_FALSE(status);

    ret = kvDelegate->Delete(KvDBDelegate::TEMPLATE_TABLE,
        static_cast<std::string>(Id(TemplateData::GenId(key.uri, key.bundleName, key.subscriberId), USER_TEST)));
    EXPECT_EQ(ret.first, OHOS::DataShare::E_OK);
    EXPECT_GT(ret.second, 0);
    ZLOGI("DisableScheduler001 end");
}
} // namespace OHOS::Test
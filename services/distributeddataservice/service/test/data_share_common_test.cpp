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
#define LOG_TAG "DataShareCommonTest"

#include <gtest/gtest.h>
#include <unistd.h>
#include "extension_connect_adaptor.h"
#include "data_share_profile_config.h"
#include "div_strategy.h"
#include "log_print.h"
#include "rdb_delegate.h"
#include "rdb_subscriber_manager.h"
#include "scheduler_manager.h"
#include "seq_strategy.h"
#include "strategy.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DataShareCommonTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: DivStrategy001
* @tc.desc: test DivStrategy function when three parameters are all nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy001, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy001 start");
    std::shared_ptr<Strategy> check = nullptr;
    std::shared_ptr<Strategy> trueAction = nullptr;
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy001 end");
}

/**
* @tc.name: DivStrategy002
* @tc.desc: test DivStrategy function when trueAction and falseAction are nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy002, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy002 start");
    std::shared_ptr<Strategy> check = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> trueAction = nullptr;
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy002 end");
}

/**
* @tc.name: DivStrategy003
* @tc.desc: test DivStrategy function when only falseAction is nullptr
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareCommonTest, DivStrategy003, TestSize.Level1)
{
    ZLOGI("DataShareCommonTest DivStrategy003 start");
    std::shared_ptr<Strategy> check = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> trueAction = std::make_shared<Strategy>();
    std::shared_ptr<Strategy> falseAction = nullptr;
    DivStrategy divStrategy(check, trueAction, falseAction);
    auto context = std::make_shared<Context>();
    bool result = divStrategy.operator()(context);
    EXPECT_EQ(result, false);
    ZLOGI("DataShareCommonTest DivStrategy003 end");
}

/**
* @tc.name: SeqStrategyInit001
* @tc.desc: test SeqStrategyInit function when item is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a seqStrategy object and strategy2 = nullptr
    2.call Init function
* @tc.experct: Init failed and return false
*/
HWTEST_F(DataShareCommonTest, SeqStrategyInit001, TestSize.Level1)
{
    ZLOGI("SeqStrategyInit001 start");
    Strategy* strategy1 = new Strategy();
    Strategy* strategy2 = nullptr;
    SeqStrategy seqStrategy;
    bool result = seqStrategy.Init({strategy1, strategy2});
    EXPECT_FALSE(result);
    delete strategy1;
    ZLOGI("SeqStrategyInit001 end");
}

/**
* @tc.name: DoConnect001
* @tc.desc: test DoConnect function when callback_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a ExtensionConnectAdaptor object and callback = nullptr
    2.call DoConnect function
* @tc.experct: DoConnect failed and return false
*/
HWTEST_F(DataShareCommonTest, DoConnect001, TestSize.Level1)
{
    ZLOGI("DoConnect001 start");
    std::string uri = "testUri";
    std::string bundleName = "testBundle";
    AAFwk::WantParams wantParams;
    ExtensionConnectAdaptor extensionConnectAdaptor;
    bool result = extensionConnectAdaptor.DoConnect(uri, bundleName, wantParams);
    EXPECT_FALSE(result);
    ZLOGI("DoConnect001 end");
}

/**
* @tc.name: InsertEx001
* @tc.desc: test InsertEx function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a rdbDelegate object and store_ = nullptr
    2.call InsertEx function
* @tc.experct: InsertEx failed and return E_DB_ERROR
*/
HWTEST_F(DataShareCommonTest, InsertEx001, TestSize.Level1)
{
    ZLOGI("InsertEx001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string tableName = "";
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "";
    valuesBucket.Put("", name0);
    auto result = rdbDelegate.InsertEx(tableName, valuesBucket);
    EXPECT_EQ(result.first, E_DB_ERROR);
    ZLOGI("InsertEx001 end");
}

/**
* @tc.name: UpdateEx001
* @tc.desc: test UpdateEx function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call UpdateEx function
* @tc.experct: UpdateEx failed and return E_DB_ERROR
*/
HWTEST_F(DataShareCommonTest, UpdateEx001, TestSize.Level1)
{
    ZLOGI("UpdateEx001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string tableName = "";
    DataShare::DataShareValuesBucket valuesBucket;
    std::string name0 = "";
    valuesBucket.Put("", name0);
    DataSharePredicates predicate;
    auto result = rdbDelegate.UpdateEx(tableName, predicate, valuesBucket);
    EXPECT_EQ(result.first, E_DB_ERROR);
    ZLOGI("UpdateEx001 end");
}

/**
* @tc.name: DeleteEx001
* @tc.desc: test DeleteEx function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call DeleteEx function
* @tc.experct: DeleteEx failed and return E_DB_ERROR
*/
HWTEST_F(DataShareCommonTest, DeleteEx001, TestSize.Level1)
{
    ZLOGI("DeleteEx001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string tableName = "";
    DataSharePredicates predicate;
    auto result = rdbDelegate.DeleteEx(tableName, predicate);
    EXPECT_EQ(result.first, E_DB_ERROR);
    ZLOGI("DeleteEx001 end");
}

/**
* @tc.name: Query001
* @tc.desc: test Query function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call Query function
* @tc.experct: Query failed and return errCode
*/
HWTEST_F(DataShareCommonTest, Query001, TestSize.Level1)
{
    ZLOGI("Query001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string tableName = "";
    DataSharePredicates predicate;
    std::vector<std::string> columns;
    int32_t callingPid = 1;
    uint32_t callingTokenId = 1;
    auto result = rdbDelegate.Query(tableName, predicate, columns, callingPid, callingTokenId);
    EXPECT_EQ(result.first, rdbDelegate.errCode_);
    ZLOGI("Query001 end");
}

/**
* @tc.name: Query002
* @tc.desc: test Query function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call Query function
* @tc.experct: Query failed and return ""
*/
HWTEST_F(DataShareCommonTest, Query002, TestSize.Level1)
{
    ZLOGI("Query002 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string sql = "testsql";
    std::vector<std::string> selectionArgs;
    auto result = rdbDelegate.Query(sql, selectionArgs);
    EXPECT_EQ(result, "");
    ZLOGI("Query002 end");
}

/**
* @tc.name: QuerySql001
* @tc.desc: test QuerySql function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call QuerySql function
* @tc.experct: QuerySql failed and return nullptr
*/
HWTEST_F(DataShareCommonTest, QuerySql001, TestSize.Level1)
{
    ZLOGI("QuerySql001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string sql = "testsql";
    auto result = rdbDelegate.QuerySql(sql);
    EXPECT_EQ(result, nullptr);
    ZLOGI("QuerySql001 end");
}

/**
* @tc.name: UpdateSql001
* @tc.desc: test UpdateSql function when store_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a RdbDelegate object and store_ = nullptr
    2.call UpdateSql function
* @tc.experct: UpdateSql failed and return E_ERROR
*/
HWTEST_F(DataShareCommonTest, UpdateSql001, TestSize.Level1)
{
    ZLOGI("UpdateSql001 start");
    DistributedData::StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "test";
    metaData.area = 1;
    int version = 0;
    bool registerFunction = false;
    std::string extUri = "uri";
    std::string backup = "backup";
    RdbDelegate rdbDelegate;
    rdbDelegate.Init(metaData, version, registerFunction, extUri, backup);
    rdbDelegate.store_ = nullptr;
    std::string sql = "testsql";
    auto result = rdbDelegate.UpdateSql(sql);
    EXPECT_EQ(result.first, OHOS::DataShare::E_ERROR);
    ZLOGI("UpdateSql001 end");
}

/**
* @tc.name: EraseTimerTaskId001
* @tc.desc: test EraseTimerTaskId function when not find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and timerCache_ is null
    2.call EraseTimerTaskId function
* @tc.experct: not find key, EraseTimerTaskId failed and return -1
*/
HWTEST_F(DataShareCommonTest, EraseTimerTaskId001, TestSize.Level1)
{
    ZLOGI("EraseTimerTaskId001 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    int64_t result = manager.EraseTimerTaskId(key);
    EXPECT_EQ(result, -1);
    ZLOGI("EraseTimerTaskId001 end");
}

/**
* @tc.name: EraseTimerTaskId002
* @tc.desc: test EraseTimerTaskId function when find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and timerCache_[key] = timeid
    2.call EraseTimerTaskId function
* @tc.experct: find key, EraseTimerTaskId success and return timeid
*/
HWTEST_F(DataShareCommonTest, EraseTimerTaskId002, TestSize.Level1)
{
    ZLOGI("EraseTimerTaskId002 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    int64_t timeid = 1;
    manager.timerCache_[key] = timeid;
    int64_t result = manager.EraseTimerTaskId(key);
    EXPECT_EQ(result, timeid);
    ZLOGI("EraseTimerTaskId002 end");
}

/**
* @tc.name: GetSchedulerStatus001
* @tc.desc: test GetSchedulerStatus function when not find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and schedulerStatusCache_ is null
    2.call GetSchedulerStatus function
* @tc.experct: not find key, GetSchedulerStatus failed and return false
*/
HWTEST_F(DataShareCommonTest, GetSchedulerStatus001, TestSize.Level1)
{
    ZLOGI("GetSchedulerStatus001 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    bool result = manager.GetSchedulerStatus(key);
    EXPECT_FALSE(result);
    ZLOGI("GetSchedulerStatus001 end");
}

/**
* @tc.name: GetSchedulerStatus002
* @tc.desc: test GetSchedulerStatus function when find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and schedulerStatusCache_[key] = true;
    2.call GetSchedulerStatus function
* @tc.experct: find key, GetSchedulerStatus success and return true
*/
HWTEST_F(DataShareCommonTest, GetSchedulerStatus002, TestSize.Level1)
{
    ZLOGI("GetSchedulerStatus002 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.schedulerStatusCache_[key] = true;
    bool result = manager.GetSchedulerStatus(key);
    EXPECT_TRUE(result);
    ZLOGI("GetSchedulerStatus002 end");
}

/**
* @tc.name: RemoveTimer001
* @tc.desc: test RemoveTimer function when executor_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object what timerCache_[key] = 1 and executor_ is nullptr
    2.call RemoveTimer function to remove timer
* @tc.experct: RemoveTimer failed and timerCache_ is not null
*/
HWTEST_F(DataShareCommonTest, RemoveTimer001, TestSize.Level1)
{
    ZLOGI("RemoveTimer001 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.executor_ = nullptr;
    manager.timerCache_[key] = 1;
    manager.RemoveTimer(key);
    EXPECT_NE(manager.timerCache_.find(key), manager.timerCache_.end());
    ZLOGI("RemoveTimer001 end");
}

/**
* @tc.name: RemoveTimer002
* @tc.desc: test RemoveTimer function when executor_ is not nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and timerCache_[key] = 1 and executor_ is not nullptr
    2.call RemoveTimer function to remove timer
* @tc.experct: RemoveTimer success and timerCache_ is null
*/
HWTEST_F(DataShareCommonTest, RemoveTimer002, TestSize.Level1)
{
    ZLOGI("RemoveTimer002 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.executor_ = std::make_shared<ExecutorPool>(5, 3);
    manager.timerCache_[key] = 1;
    manager.RemoveTimer(key);
    EXPECT_EQ(manager.timerCache_.find(key), manager.timerCache_.end());
    ZLOGI("RemoveTimer002 end");
}

/**
* @tc.name: ClearTimer001
* @tc.desc: test ClearTimer function when executor_ is nullptr
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object what timerCache_[key] = 1 and executor_ is nullptr
    2.call ClearTimer function to clear timer
* @tc.experct: ClearTimer failed and timerCache_ is not empty
*/
HWTEST_F(DataShareCommonTest, ClearTimer001, TestSize.Level1)
{
    ZLOGI("ClearTimer001 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.executor_ = nullptr;
    manager.timerCache_[key] = 1;
    manager.ClearTimer();
    EXPECT_FALSE(manager.timerCache_.empty());
    ZLOGI("ClearTimer001 end");
}

/**
* @tc.name: ClearTimer002
* @tc.desc: test ClearTimer function when executor_ is  not nullptr and find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and timerCache_[key] = 1 and executor_ is not nullptr
    2.call ClearTimer function to clear timer
* @tc.experct: ClearTimer success and timerCache_ is empty
*/
HWTEST_F(DataShareCommonTest, ClearTimer002, TestSize.Level1)
{
    ZLOGI("ClearTimer002 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.executor_ = std::make_shared<ExecutorPool>(5, 3);
    manager.timerCache_[key] = 1;
    manager.ClearTimer();
    EXPECT_TRUE(manager.timerCache_.empty());
    ZLOGI("ClearTimer002 end");
}

/**
* @tc.name: ClearTimer003
* @tc.desc: test ClearTimer function when executor_ is  not nullptr and not find key
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a SchedulerManager object and timerCache_ is null
    2.call ClearTimer function
* @tc.experct: ClearTimer failed and timerCache_ is empty
*/
HWTEST_F(DataShareCommonTest, ClearTimer003, TestSize.Level1)
{
    ZLOGI("ClearTimer003 start");
    Key key("uri1", 1, "name1");
    auto& manager = SchedulerManager::GetInstance();
    manager.executor_ = std::make_shared<ExecutorPool>(5, 3);
    manager.ClearTimer();
    EXPECT_TRUE(manager.timerCache_.empty());
    ZLOGI("ClearTimer003 end");
}

/**
 * @tc.name: DBDelegateTest001
 * @tc.desc: do nothing when delegate already inited
 * @tc.type: FUNC
 */
HWTEST_F(DataShareCommonTest, DBDelegateTest001, TestSize.Level0) {
    RdbDelegate delegate;
    DistributedData::StoreMetaData meta;
    meta.tokenId = 1;

    delegate.isInited_ = true;
    delegate.Init({}, 0, false, "extUri", "backup");

    EXPECT_TRUE(delegate.isInited_);
    EXPECT_EQ(delegate.tokenId_, 0);
    EXPECT_EQ(delegate.extUri_, "");
    EXPECT_EQ(delegate.backup_, "");
}

/**
 * @tc.name  : DBDelegateTest002
 * @tc.number: init members in delegate init
 * @tc.type: FUNC
 */
HWTEST_F(DataShareCommonTest, DBDelegateTest002, TestSize.Level0) {
    RdbDelegate delegate;
    DistributedData::StoreMetaData meta;
    meta.tokenId = 1;

    delegate.isInited_ = false;
    delegate.Init(meta, 0, false, "extUri", "backup");

    EXPECT_FALSE(delegate.isInited_);
    EXPECT_EQ(delegate.tokenId_, meta.tokenId);
    EXPECT_EQ(delegate.extUri_, "extUri");
    EXPECT_EQ(delegate.backup_, "backup");
}
} // namespace OHOS::Test
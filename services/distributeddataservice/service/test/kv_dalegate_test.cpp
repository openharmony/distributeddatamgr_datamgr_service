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
#define LOG_TAG "KvDelegateTest"

#include <gtest/gtest.h>
#include <unistd.h>
#include "data_share_profile_config.h"
#include "executor_pool.h"
#include "grd_error.h"
#include "kv_delegate.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class KvDelegateTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

const char* g_backupFiles[] = {
    "dataShare.db",
    "dataShare.db.redo",
    "dataShare.db.safe",
    "dataShare.db.undo",
};
const char* BACKUP_SUFFIX = ".backup";
std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(5, 3);

/**
* @tc.name: RestoreIfNeedt001
* @tc.desc: test RestoreIfNeed function when dbstatus is GRD_INVALID_FILE_FORMAT
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(KvDelegateTest, RestoreIfNeed001, TestSize.Level1)
{
    ZLOGI("KvDelegateTest RestoreIfNeed001 start");
    int32_t dbstatus = GRD_INVALID_FILE_FORMAT;
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    bool result = kvDelegate.RestoreIfNeed(dbstatus);
    EXPECT_EQ(result, true);
    ZLOGI("KvDelegateTest RestoreIfNeed001 end");
}

/**
* @tc.name: RestoreIfNeedt002
* @tc.desc: test RestoreIfNeed function when dbstatus is GRD_REBUILD_DATABASE
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(KvDelegateTest, RestoreIfNeed002, TestSize.Level1)
{
    ZLOGI("KvDelegateTest RestoreIfNeed002 start");
    int32_t dbstatus = GRD_REBUILD_DATABASE;
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    bool result = kvDelegate.RestoreIfNeed(dbstatus);
    EXPECT_EQ(result, true);
    ZLOGI("KvDelegateTest RestoreIfNeed002 end");
}

/**
* @tc.name: RestoreIfNeedt003
* @tc.desc: test RestoreIfNeed function when dbstatus is GRD_TIME_OUT
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(KvDelegateTest, RestoreIfNeed003, TestSize.Level1)
{
    ZLOGI("KvDelegateTest RestoreIfNeed003 start");
    int32_t dbstatus = GRD_TIME_OUT;
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    bool result = kvDelegate.RestoreIfNeed(dbstatus);
    EXPECT_EQ(result, false);
    ZLOGI("KvDelegateTest RestoreIfNeed003 end");
}

/**
* @tc.name: GetVersion001
* @tc.desc: test GetVersion function when get version failed
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(KvDelegateTest, GetVersion001, TestSize.Level1)
{
    ZLOGI("KvDelegateTest GetVersion001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    std::string collectionname = "testname";
    std::string filter = "testfilter";
    int version = 0;
    bool result = kvDelegate.GetVersion(collectionname, filter, version);
    EXPECT_EQ(result, false);
    ZLOGI("KvDelegateTest GetVersion001 end");
}

/**
* @tc.name: Upsert001
* @tc.desc: test Upsert function when GDR_DBOpen failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= false
    2.call Upsert function to upsert filter
* @tc.experct: Upsert failed and return E_ERROR
*/
HWTEST_F(KvDelegateTest, Upsert001, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Upsert001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string value = "value";
    auto result = kvDelegate.Upsert(collectionName, filter, value);
    EXPECT_EQ(result.first, E_ERROR);
    ZLOGI("KvDelegateTest Upsert001 end");
}

/**
* @tc.name: Upsert002
* @tc.desc: test Upsert function when GRD_UpsertDoc failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= true
    2.call Upsert function to upsert filter
* @tc.experct: Upsert failed and return GRD_INVALID_ARGS
*/
HWTEST_F(KvDelegateTest, Upsert002, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Upsert001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.isInitDone_= true;
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string value = "value";
    auto result = kvDelegate.Upsert(collectionName, filter, value);
    EXPECT_EQ(result.first, GRD_INVALID_ARGS);
    ZLOGI("KvDelegateTest Upsert002 end");
}

/**
* @tc.name: Delete001
* @tc.desc: test Delete function when GDR_DBOpen failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= false
    2.call Delete function to Delete filter
* @tc.experct: Delete failed and return E_ERROR
*/
HWTEST_F(KvDelegateTest, Delete001, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Delete001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    std::string collectionName = "test";
    std::string filter = "filter";
    auto result = kvDelegate.Delete(collectionName, filter);
    EXPECT_EQ(result.first, E_ERROR);
    ZLOGI("KvDelegateTest Delete001 end");
}

/**
* @tc.name: Delete002
* @tc.desc: test Delete function when GRD_DeleteDoc failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= true
    2.call Delete function to Delete filter
* @tc.experct: Delete failed and return GRD_INVALID_ARGS
*/
HWTEST_F(KvDelegateTest, Delete002, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Delete002 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.isInitDone_= true;
    std::string collectionName = "test";
    std::string filter = "filter";
    auto result = kvDelegate.Delete(collectionName, filter);
    EXPECT_EQ(result.first, GRD_INVALID_ARGS);
    ZLOGI("KvDelegateTest Delete002 end");
}

/**
* @tc.name: Init001
* @tc.desc: test Init function when isInitDone_ = true
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= true
    2.call Init function
* @tc.experct: Init failed and return true
*/
HWTEST_F(KvDelegateTest, Init001, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Init001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.isInitDone_ = true;
    auto result = kvDelegate.Init();
    EXPECT_TRUE(result);
    ZLOGI("KvDelegateTest Init001 end");
}

/**
* @tc.name: Init002
* @tc.desc: test Init function when isInitDone_ = false
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= false
    2.call Init function
* @tc.experct: Init failed and return false
*/
HWTEST_F(KvDelegateTest, Init002, TestSize.Level1)
{
    ZLOGI("KvDelegateTest Init002 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    auto result = kvDelegate.Init();
    EXPECT_FALSE(result);
    ZLOGI("KvDelegateTest Init002 end");
}

/**
* @tc.name: Get001
* @tc.desc: test Get function when GDR_DBOpen failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= false
    2.call Get function
* @tc.experct: Get failed and return E_ERROR
*/
HWTEST_F(KvDelegateTest, Get001, TestSize.Level1)
{
    ZLOGI("Get001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string projection = "projection";
    std::string value = "value";
    std::string result = "result";
    auto result1 = kvDelegate.Get(collectionName, filter, projection, result);
    EXPECT_EQ(result1, E_ERROR);
    ZLOGI("Get001 end");
}

/**
* @tc.name: Get002
* @tc.desc: test Get function when GRD_FindDoc failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= true
    2.call Get function
* @tc.experct: Get failed and return GRD_INVALID_ARGS
*/
HWTEST_F(KvDelegateTest, Get002, TestSize.Level1)
{
    ZLOGI("Get002 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.isInitDone_ = true;
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string projection = "projection";
    std::string value = "value";
    std::string result = "result";
    auto result1 = kvDelegate.Get(collectionName, filter, projection, result);
    EXPECT_EQ(result1, GRD_INVALID_ARGS);
    ZLOGI("Get002 end");
}

/**
* @tc.name: Backup001
* @tc.desc: test Backup function when hasChange_ is true
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.hasChange_ = true
    2.call Backup function
* @tc.experct: need backup and change kvDelegate.hasChange_ to false
*/
HWTEST_F(KvDelegateTest, Backup001, TestSize.Level1)
{
    ZLOGI("Backup001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.hasChange_ = true;
    kvDelegate.Backup();
    EXPECT_FALSE(kvDelegate.hasChange_);
    ZLOGI("Backup001 end");
}

/**
* @tc.name: Backup002
* @tc.desc: test Backup function when hasChange_ is false
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.hasChange_ = false
    2.call Backup function
* @tc.experct: no need to backup and kvDelegate.hasChange_ not change
*/
HWTEST_F(KvDelegateTest, Backup002, TestSize.Level1)
{
    ZLOGI("Backup002 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.Backup();
    EXPECT_FALSE(kvDelegate.hasChange_);
    ZLOGI("Backup002 end");
}

/**
* @tc.name: GetBatch001
* @tc.desc: test GetBatch function when GDR_DBOpen failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= false
    2.call GetBach function
* @tc.experct: GetBach failed and return E_ERROR
*/
HWTEST_F(KvDelegateTest, GetBatch001, TestSize.Level1)
{
    ZLOGI("GetBatch001 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string projection = "projection";
    std::string value = "value";
    std::vector<std::string> result;
    auto result1 = kvDelegate.GetBatch(collectionName, filter, projection, result);
    EXPECT_EQ(result1, E_ERROR);
    ZLOGI("GetBatch001 end");
}

/**
* @tc.name: GetBatch002
* @tc.desc: test GetBatch function when GRD_FindDoc failed
* @tc.type: FUNC
* @tc.require:issueIBX9E1
* @tc.precon: None
* @tc.step:
    1.Creat a kvDelegate object and kvDelegate.isInitDone_= true
    2.call GetBatch function
* @tc.experct: GetBatch failed and return GRD_INVALID_ARGS
*/
HWTEST_F(KvDelegateTest, GetBatch002, TestSize.Level1)
{
    ZLOGI("GetBatch002 start");
    std::string path = "path/to/your/db";
    KvDelegate kvDelegate(path, executors);
    kvDelegate.isInitDone_ = true;
    std::string collectionName = "test";
    std::string filter = "filter";
    std::string projection = "projection";
    std::string value = "value";
    std::vector<std::string> result;
    auto result1 = kvDelegate.GetBatch(collectionName, filter, projection, result);
    EXPECT_EQ(result1, GRD_INVALID_ARGS);
    ZLOGI("GetBatch001 end");
}
} // namespace OHOS::Test
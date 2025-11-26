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

#include <dlfcn.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <iterator>
#include "data_share_profile_config.h"
#include "db_delegate.h"
#include "executor_pool.h"
#include "grd_base/grd_error.h"
#include "kv_delegate.h"
#include "log_print.h"
#include "proxy_data_manager.h"
#include "parameters.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class KvDelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void);
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
bool g_isRK3568 = false;
static void *g_library = nullptr;

void KvDelegateTest::SetUpTestCase(void)
{
    static std::once_flag flag;
    static std::string product;
    const std::string invalidProduct { "default" };
    std::call_once(flag, []() {
        product = OHOS::system::GetParameter("const.build.product", "");
    });
    if (product == invalidProduct)  {
        std::cout << "This test is not for " << invalidProduct << ", skip it." << std::endl;
        g_isRK3568 = true;
        return;
    }
}

bool FileComparison(const std::string& file1, const std::string& file2)
{
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    if (!f1.is_open() || !f2.is_open()) {
        ZLOGI("fail to open files!");
        return false; // fail to open files
    }

    // compare file size
    f1.seekg(0, std::ios::end);
    f2.seekg(0, std::ios::end);
    if (f1.tellg() != f2.tellg()) {
        ZLOGI("File size is different!");
        return false;
    }
    f1.seekg(0);
    f2.seekg(0);

    return std::equal(
        std::istreambuf_iterator<char>(f1),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(f2)
    );
}

bool IsUsingArkData()
{
#ifndef _WIN32
    g_library = dlopen("libarkdata_db_core.z.so", RTLD_LAZY);
    return g_library != nullptr;
#else
    return false;
#endif
}

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
* @tc.name: KVDelegateGetInstanceTest001
* @tc.desc: Test KvDelegate::GetInstance() function when both delegate and executors are null.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Call KvDBDelegate::GetInstance() to initialize kvDBDelegate.
* @tc.experct: Init failed. delegate is nullptr.
*/
HWTEST_F(KvDelegateTest, KVDelegateGetInstanceTest001, TestSize.Level0) {
    ZLOGI("KVDelegateGetInstanceTest001 start");
    auto delegate = KvDBDelegate::GetInstance("/data/test", nullptr);
    EXPECT_EQ(delegate, nullptr);
    ZLOGI("KVDelegateGetInstanceTest001 end");
}

/**
* @tc.name: KVDelegateGetInstanceTest002
* @tc.desc: Test KvDelegate::GetInstance() function when delegate is null and executors is not null.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Call KvDBDelegate::GetInstance() to initialize kvDBDelegate.
    2.Call KvDBDelegate::GetInstance() to get kvDelegate when delegate is already initialized.
* @tc.experct: Init failed. delegate is nullptr.
*/
HWTEST_F(KvDelegateTest, KVDelegateGetInstanceTest002, TestSize.Level0) {
    ZLOGI("KVDelegateGetInstanceTest002 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = KvDBDelegate::GetInstance("/data/test", executors);
    EXPECT_NE(delegate, nullptr);
    delegate = KvDBDelegate::GetInstance("/data/test", executors);
    EXPECT_NE(delegate, nullptr);
    ZLOGI("KVDelegateGetInstanceTest002 end");
}

/**
* @tc.name: KVDelegateGetInstanceTest003
* @tc.desc: Test KvDelegate::GetInstance() function when delegate is not null and executors is null.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Call KvDBDelegate::GetInstance() to initialize kvDBDelegate.
    2.Call KvDBDelegate::GetInstance() with empty executors to get kvDelegate when delegate is already initialized
* @tc.experct: Init failed. delegate is nullptr.
*/
HWTEST_F(KvDelegateTest, KVDelegateGetInstanceTest003, TestSize.Level0) {
    ZLOGI("KVDelegateGetInstanceTest003 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = KvDBDelegate::GetInstance("/data/test", executors);
    EXPECT_NE(delegate, nullptr);
    delegate = KvDBDelegate::GetInstance("/data/test", nullptr);
    EXPECT_NE(delegate, nullptr);
    ZLOGI("KVDelegateGetInstanceTest003 end");
}

/**
* @tc.name: KVDelegateResetBackupTest001
* @tc.desc: Test KvDelegate::Init() when does not satisfy reset schedule conditions. Insert two datasets with the
    second time waitTime_ set to 3600s. First scheduled backup waitTime is 1s. absoluteWaitTime_ is set
    to 0ms to make sure backup schedule will not be reset to wait another 3600, First scheduled backup should be
    done and not reset, and second inserted data should be included in the backup process, thus the backup
    database should remain same as the original database with two dataset.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Initilize a KvDelegate object.
    2.set waitTime_ to 1s.
    3.call Upsert() to insert a dataset.
    4.Call NotifyBackup() to set hasChange_ condition to true to allow backup.
    5.Keep scheduled backup from being reset by setting absoluteWaitTime_ to 0ms.
    6.Set waitTime_ to 3600s to make sure if backup schedule resets, it will be reset to 3600s.
    7.call Upsert() to insert another dataset.
    8.Call NotifyBackup() to set hasChange_ condition to true to allow backup.
* @tc.experct: backup file should be the same as original file.
*/
HWTEST_F(KvDelegateTest, KVDelegateResetBackupTest001, TestSize.Level0) {
    if (g_isRK3568 || !IsUsingArkData()) {
        GTEST_SKIP();
    }
    ZLOGI("KVDelegateResetBackupTest001 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = new KvDelegate("/data/test", executors);
    delegate->waitTime_ = 2; // 1s
    std::vector<std::string> proxyDataList = {"name", "age", "job"};
    std::vector<std::string> proxyDataList1 = {"test", "hellow", "world"};
    auto value = ProxyDataList(ProxyDataListNode(proxyDataList, 24, 12));
    auto value1 = ProxyDataList(ProxyDataListNode(proxyDataList1, 10, 24));
    auto result = delegate->Upsert("proxydata_", value);
    delegate->NotifyBackup();
    EXPECT_EQ(result.first, 0);

    // Set absoluteWaitTime_ to 0ms to make sure backup schedule will not be reset to wait another 3600s.
    delegate->absoluteWaitTime_ = 0;
    delegate->waitTime_ = 3600;
    auto result1 = delegate->Upsert("proxydata_", value1);
    delegate->NotifyBackup();
    EXPECT_EQ(result1.first, 0);
    // First scheduled backup waitTime is 1s. Margin of error is 1s.
    sleep(2);
    // Backup shoud be done because schdule did not reset.
    for (auto &fileName : g_backupFiles) {
        std::string src = delegate->path_ + "/" + fileName;
        std::string dst = src;
        dst.append(BACKUP_SUFFIX);
        EXPECT_TRUE(FileComparison(src, dst));
    }
    ZLOGI("KVDelegateResetBackupTest001 end");
}

/**
* @tc.name: KVDelegateRestoreTest001
* @tc.desc: Test RestoreIfNeed() after backup is done
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Creat a KvDelegate object with an initialized executorPool.
    2.Set waitTime_ to 2s.
    3.Call Upsert() to insert a dataset.
    4.Get the dataset just inserted.
    5.Call NotifyBackup() to set hasChange_ condition to true to allow backup.
    6.Call FileComparison() to compare original database with backup database.
    7.Delete original database and call RestoreIfNeed() with GRD_REBUILD_DATABASE as input param.
* @tc.experct: Successfully Get the dataset previously inserted.
*/
HWTEST_F(KvDelegateTest, KVDelegateRestoreTest001, TestSize.Level1) {
    if (g_isRK3568 || !IsUsingArkData()) {
        GTEST_SKIP();
    }
    ZLOGI("KVDelegateRestoreTest001 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = new KvDelegate("/data/test", executors);
    delegate->waitTime_ = 1;
    std::vector<std::string> proxyDataList = {"name", "age", "job"};
    auto value = ProxyDataList(ProxyDataListNode(proxyDataList, 24, 12));
    auto result = delegate->Upsert("proxydata_", value);
    delegate->NotifyBackup();
    EXPECT_EQ(result.first, 0);
    Id id = Id(std::to_string(12), 24);
    std::string queryResult = "";

    auto result1 = delegate->Get("proxydata_", id, queryResult);

    EXPECT_EQ(result1, 0);
    // Scheduled backup waitTime is 1s, 1s for margin of error.
    sleep(2);
    for (auto &fileName : g_backupFiles) {
        std::string src = delegate->path_ + "/" + fileName;
        std::string dst = src;
        dst.append(BACKUP_SUFFIX);
        EXPECT_TRUE(FileComparison(src, dst));
    }

    EXPECT_EQ(std::remove("/data/test/dataShare.db"), 0);
    bool restoreRet = delegate->RestoreIfNeed(GRD_REBUILD_DATABASE);
    EXPECT_TRUE(restoreRet);

    std::string queryRetAfterRestore = "";
    result1 = delegate->Get("proxydata_", id, queryRetAfterRestore);
    EXPECT_EQ(result1, 0);
    ZLOGI("KVDelegateRestoreTest001 end");
}

/**
* @tc.name: KVDelegateInitTest001
* @tc.desc: Test KvDelegate::Init() function when executors_ are null.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Initilize a KvDelegate object.
    2.Set executors_ to nullptr.
    3.Call Init() function.
    4.Call Init() function again.
* @tc.experct: Database init success, db_ is not nullptr.
*/
HWTEST_F(KvDelegateTest, KVDelegateInitTest001, TestSize.Level0) {
    ZLOGI("KVDelegateInitTest001 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = new KvDelegate("/data/test", executors);
    EXPECT_NE(delegate, nullptr);
    delegate->executors_ = nullptr;
    bool result = delegate->Init();
    EXPECT_TRUE(result);
    EXPECT_NE(delegate->db_, nullptr);
    EXPECT_TRUE(delegate->isInitDone_);
    EXPECT_EQ(delegate->absoluteWaitTime_, 0);
    bool result1 = delegate->Init();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(delegate->isInitDone_);
    ZLOGI("KVDelegateInitTest001 end");
}

/**
* @tc.name: KVDelegateInitTest002
* @tc.desc: Test KvDelegate::Init() function when executors_ is not null.
* @tc.type: FUNC
* @tc.require:issueICNFIF
* @tc.precon: None
* @tc.step:
    1.Initilize a KvDelegate object.
    2.call Init() function.
* @tc.experct: Init success. Schedule success, taskId is not invalid.
*/
HWTEST_F(KvDelegateTest, KVDelegateInitTest002, TestSize.Level0) {
    ZLOGI("KVDelegateInitTest002 start");
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    auto delegate = new KvDelegate("/data/test", executors);
    bool result = delegate->Init();
    EXPECT_TRUE(result);
    EXPECT_NE(delegate->db_, nullptr);
    EXPECT_TRUE(delegate->isInitDone_);
    EXPECT_NE(delegate->absoluteWaitTime_, 0);
    EXPECT_NE(delegate->taskId_, ExecutorPool::INVALID_TASK_ID);
    ZLOGI("KVDelegateInitTest002 end");
}
} // namespace OHOS::Test
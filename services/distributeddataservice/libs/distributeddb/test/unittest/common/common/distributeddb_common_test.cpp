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

#include "db_errno.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    std::string g_testDir;

    // define some variables to init a KvStoreDelegateManager object.
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;

    // define the g_kvDelegateCallback, used to get some information when open a kv store.
    DBStatus g_kvDelegateStatus = INVALID_ARGS;

    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        std::placeholders::_1, std::placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));
}

class DistributedDBCommonTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCommonTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
}

void DistributedDBCommonTest::TearDownTestCase(void) {}

void DistributedDBCommonTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
}

void DistributedDBCommonTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGI("rm test db files error!");
    }
}

/**
 * @tc.name: RemoveAllFilesOfDirectory
 * @tc.desc: Test delete all file and dir.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: sunpeng
 */
HWTEST_F(DistributedDBCommonTest, RemoveAllFilesOfDirectory, TestSize.Level1)
{
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/dirLevel1_1/"), E_OK);
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/dirLevel1_1/" + "/dirLevel2_1/"), E_OK);
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/dirLevel1_1/" + "/dirLevel2_2/"), E_OK);
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/dirLevel1_1/" + "/dirLevel2_2/" + "/dirLevel3_1/"), E_OK);

    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/fileLevel1_1"), E_OK);
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/dirLevel1_1/" + "/fileLevel2_1"), E_OK);
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/dirLevel1_1/" + "/dirLevel2_2/" +
        "/dirLevel3_1/"+ "/fileLevel4_1/"), E_OK);

    EXPECT_EQ(DBCommon::RemoveAllFilesOfDirectory(g_testDir), E_OK);

    EXPECT_EQ(OS::CheckPathExistence(g_testDir), false);
}

#ifdef RUNNING_ON_LINUX
/**
 * @tc.name: SameProcessReLockFile
 * @tc.desc: Test same process repeat lock same file.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: sunpeng
 */
HWTEST_F(DistributedDBCommonTest, SameProcessReLockFile, TestSize.Level1)
{
    // block mode
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/blockmode"), E_OK);
    OS::FileHandle fd;
    EXPECT_EQ(OS::OpenFile(g_testDir + "/blockmode", fd), E_OK);

    EXPECT_EQ(OS::FileLock(fd, true), E_OK);
    EXPECT_EQ(OS::FileLock(fd, true), E_OK);

    // normal mode
    OS::FileHandle fd2;
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/normalmode"), E_OK);
    EXPECT_EQ(OS::OpenFile(g_testDir + "/normalmode", fd2), E_OK);
    EXPECT_EQ(OS::FileLock(fd2, true), E_OK);
    EXPECT_EQ(OS::FileLock(fd2, true), E_OK);

    // unlock
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
    EXPECT_EQ(OS::FileUnlock(fd2), E_OK); // unlock success will close fd
}

/**
 * @tc.name: SameProcessReUnLockFile
 * @tc.desc: Test same process repeat lock same file.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: sunpeng
 */
HWTEST_F(DistributedDBCommonTest, SameProcessReUnLockFile, TestSize.Level1)
{
    // unlock normal file twice
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/normalmode"), E_OK);
    OS::FileHandle fd;
    EXPECT_EQ(OS::OpenFile(g_testDir + "/normalmode", fd), E_OK);
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);  // unlock success will close fd

    EXPECT_EQ(OS::FileLock(fd, true), -E_SYSTEM_API_FAIL);
    EXPECT_EQ(OS::FileLock(fd, true), -E_SYSTEM_API_FAIL);
    ASSERT_EQ(fd.handle, -1);

    // block mode
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/blockmode"), E_OK);
    EXPECT_EQ(OS::OpenFile(g_testDir + "/blockmode", fd), E_OK);

    EXPECT_EQ(OS::FileLock(fd, false), E_OK);
    EXPECT_EQ(OS::FileLock(fd, false), E_OK);

    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
}

/**
 * @tc.name: CalcFileSizeTest
 * @tc.desc: Test the file size for function test and the performance test.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: wangbingquan
 */
HWTEST_F(DistributedDBCommonTest, CalcFileSizeTest, TestSize.Level1)
{
    std::string filePath = g_testDir + "/testFileSize";
    std::ofstream ofs(filePath, std::ofstream::out);
    ASSERT_TRUE(ofs.good());
    ofs << "test file size";
    ofs.close();
    uint64_t fileSize = 0;
    EXPECT_EQ(OS::CalFileSize(filePath, fileSize), E_OK);
    EXPECT_GT(fileSize, 0ULL);
    EXPECT_EQ(OS::RemoveFile(filePath), E_OK);
}

// Distributed db is not recommended to use multiple processes to access
// This testcase only guard for some wrong use on current product
#if defined(RUN_MULTI_PROCESS_TEST)
namespace {
// use file sync diff process information
bool waitForStep(int step, int retryTimes)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    while (retryTimes >= 0 && !OS::CheckPathExistence(g_testDir + "/LOCK_step_" + std::to_string(step))) {
        retryTimes = retryTimes - 1; // wait 10ms one times
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // once 10 ms
    }
    return (retryTimes > 0);
}

void createStepFlag(int step)
{
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + "/LOCK_step_" + std::to_string(step)), E_OK);
}
}

/**
 * @tc.name: DiffProcessLockFile
 * @tc.desc: Test different process repeat lock same file.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: sunpeng
 */
HWTEST_F(DistributedDBCommonTest, DiffProcessLockFile, TestSize.Level1)
{
    OS::FileHandle fd;
    EXPECT_EQ(OS::OpenFile(g_testDir + DBConstant::DB_LOCK_POSTFIX, fd), E_OK);
    EXPECT_EQ(OS::FileLock(fd, false), E_OK);
    sleep(1);
    LOGI("begin fork new process!!");
    pid_t pid = fork();
    ASSERT_TRUE(pid >= 0);
    if (pid < 0) {
        return;
    }
    else if (pid == 0) {
        LOGI("child process begin!");
        OS::FileHandle ChildFd;
        EXPECT_EQ(OS::OpenFile(g_testDir + DBConstant::DB_LOCK_POSTFIX, ChildFd), E_OK);
        ASSERT_TRUE(waitForStep(1, 10));
        EXPECT_EQ(OS::FileLock(ChildFd, false), -E_BUSY);
        createStepFlag(2);
        EXPECT_EQ(OS::CloseFile(ChildFd), E_OK);
        exit(0);
    } else {
        LOGI("main process begin!");
        EXPECT_EQ(OS::FileLock(fd, false), E_OK);
        createStepFlag(1);

        ASSERT_TRUE(waitForStep(2, 100));
        EXPECT_EQ(OS::CloseFile(fd), E_OK); // fd close, lock invalid
    }
}

/**
 * @tc.name: DiffProcessLockFileBlocked
 * @tc.desc: Test different process repeat lock same file.
 * @tc.type: FUNC
 * @tc.require: AR000FN6G9
 * @tc.author: sunpeng
 */
HWTEST_F(DistributedDBCommonTest, DiffProcessLockFileBlocked, TestSize.Level1)
{
    EXPECT_EQ(OS::CreateFileByFileName(g_testDir + DBConstant::DB_LOCK_POSTFIX), E_OK);
    OS::FileHandle fd;
    EXPECT_EQ(OS::OpenFile(g_testDir + DBConstant::DB_LOCK_POSTFIX, fd), E_OK);
    EXPECT_EQ(OS::FileLock(fd, true), E_OK);
    sleep(1);
    LOGI("begin fork new process!!");
    int count = 10; // wait 10 times 10 ms for block wait
    pid_t pid = fork();
    ASSERT_TRUE(pid >= 0);
    if (pid < 0) {
        return;
    }
    else if (pid == 0) {
        LOGI("child process begin!");
        EXPECT_FALSE(OS::CheckPathExistence(g_testDir + "/LOCK_step_1"));
        OS::FileHandle ChildFd;
        EXPECT_EQ(OS::OpenFile(g_testDir + DBConstant::DB_LOCK_POSTFIX, ChildFd), E_OK);
        EXPECT_EQ(OS::FileLock(ChildFd, true), E_OK);
        createStepFlag(1);
        EXPECT_EQ(OS::FileUnlock(ChildFd), E_OK);
        LOGI("child process finish!");
        exit(0);
    } else {
        LOGI("main process begin!");
        while (count--) {
            LOGI("main process waiting!");
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // once 10 ms
        }
        ASSERT_FALSE(waitForStep(1, 10));
        EXPECT_EQ(OS::FileUnlock(fd), E_OK);
        ASSERT_TRUE(waitForStep(1, 10));
    }
}

/**
  * @tc.name: DiffProcessGetDBBlocked
  * @tc.desc: Test block other process get kvstore when db locked.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessGetDBBlocked, TestSize.Level1)
{
    std::string storeId = "DiffProcessGetDBBlocked";
    std::string origId = USER_ID + "-" + APP_ID + "-" + storeId;
    std::string identifier = DBCommon::TransferHashString(origId);
    std::string hexDir = DBCommon::TransferStringToHex(identifier);
    std::string lockFile = g_testDir + "/" + hexDir + DBConstant::DB_LOCK_POSTFIX;
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/" + hexDir), E_OK);
    EXPECT_EQ(OS::CreateFileByFileName(lockFile), E_OK);
    LOGI("Create lock file[%s]", lockFile.c_str());

    LOGI("begin fork new process!!");
    pid_t pid = fork();
    OS::FileHandle fd;
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        ASSERT_TRUE(waitForStep(1, 10));
        KvStoreNbDelegate::Option option = {true, false, false};
        option.isNeedIntegrityCheck = true;
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        EXPECT_TRUE(g_kvDelegateStatus == BUSY);
        ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
        createStepFlag(2);
        exit(0);
    } else {
        LOGI("main process begin!");
        EXPECT_EQ(OS::OpenFile(lockFile, fd), E_OK);
        EXPECT_EQ(OS::FileLock(fd, false), E_OK);
        createStepFlag(1);
    }

    // Prevent the child process from not being completed, the main process ends to clean up resources
    EXPECT_TRUE(waitForStep(2, 1000));
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
}

/**
  * @tc.name: DiffProcessDeleteDBBlocked
  * @tc.desc: Test block other process delete kvstore when db locked.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessDeleteDBBlocked, TestSize.Level1)
{
    std::string storeId = "DiffProcessDeleteDBBlocked";
    std::string origId = USER_ID + "-" + APP_ID + "-" + storeId;
    std::string identifier = DBCommon::TransferHashString(origId);
    std::string hexDir = DBCommon::TransferStringToHex(identifier);
    std::string lockFile = g_testDir + "/" + hexDir + DBConstant::DB_LOCK_POSTFIX;
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/" + hexDir), E_OK);
    EXPECT_EQ(OS::CreateFileByFileName(lockFile), E_OK);
    LOGI("Create lock file[%s]", lockFile.c_str());

    KvStoreNbDelegate::Option option = {true, false, false};
    option.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    LOGI("begin fork new process!!");
    pid_t pid = fork();
    OS::FileHandle fd;
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        ASSERT_TRUE(waitForStep(1, 10));
        EXPECT_EQ(g_mgr.DeleteKvStore(storeId), BUSY);
        createStepFlag(2);
        exit(0);
    } else {
        LOGI("main process begin!");
        EXPECT_EQ(OS::OpenFile(lockFile, fd), E_OK);
        EXPECT_EQ(OS::FileLock(fd, false), E_OK);
        createStepFlag(1);
    }

    // Prevent the child process from not being completed, the main process ends to clean up resources
    EXPECT_TRUE(waitForStep(2, 1000));
    EXPECT_EQ(OS::FileUnlock(fd), E_OK);
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
}

/**
  * @tc.name: DiffProcessGetDBBlocked001
  * @tc.desc: Test block other process get kvstore when db locked.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessGetDBBlocked001, TestSize.Level1)
{
    std::string storeId = "DiffProcessGetDBBlocked001";
    std::string origId = USER_ID + "-" + APP_ID + "-" + storeId;
    std::string identifier = DBCommon::TransferHashString(origId);
    std::string hexDir = DBCommon::TransferStringToHex(identifier);
    std::string lockFile = g_testDir + "/" + hexDir + DBConstant::DB_LOCK_POSTFIX;
    EXPECT_EQ(DBCommon::CreateDirectory(g_testDir + "/" + hexDir), E_OK);
    EXPECT_EQ(OS::CreateFileByFileName(lockFile), E_OK);
    LOGI("Create lock file[%s]", lockFile.c_str());

    LOGI("begin fork new process!!");
    pid_t pid = fork();
    OS::FileHandle fd;
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        ASSERT_TRUE(waitForStep(1, 10));
        KvStoreNbDelegate::Option option = {true, false, false};
        option.isNeedIntegrityCheck = true;
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_TRUE(g_kvDelegateStatus == OK);
        createStepFlag(2);
        exit(0);
    } else {
        LOGI("main process begin!");
        EXPECT_EQ(OS::OpenFile(lockFile, fd), E_OK);
        EXPECT_EQ(OS::FileLock(fd, false), E_OK);
        createStepFlag(1);
    }
    ASSERT_TRUE(waitForStep(1, 100));

    EXPECT_EQ(OS::FileUnlock(fd), E_OK);

    ASSERT_TRUE(waitForStep(2, 100));
}

/**
  * @tc.name: DiffProcessGetDB
  * @tc.desc: Test block other process get kvstore.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessGetDB, TestSize.Level1)
{
    std::string storeId = "DiffProcessGetDB";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.isNeedIntegrityCheck = true;
    LOGI("begin fork new process!!");
    pid_t pid = fork();
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_TRUE(g_kvDelegateStatus == OK);
        createStepFlag(2);
        EXPECT_TRUE(waitForStep(1, 1000));
        exit(0);
    } else {
        LOGI("main process begin!");
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_TRUE(g_kvDelegateStatus == OK);
        createStepFlag(1);
    }
    EXPECT_TRUE(waitForStep(2, 100));
    // Prevent the child process from not being completed, the main process ends to clean up resources
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
}

/**
  * @tc.name: DiffProcessDeleteDB
  * @tc.desc: Test block other process delete kvstore.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessDeleteDB, TestSize.Level1)
{
    std::string storeId = "DiffProcessGetDB";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    LOGI("begin fork new process!!");
    pid_t pid = fork();
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_TRUE(g_kvDelegateStatus == OK);
        createStepFlag(2);
        EXPECT_TRUE(waitForStep(1, 1000));
        exit(0);
    } else {
        LOGI("main process begin!");
        g_mgr.DeleteKvStore(storeId);
        createStepFlag(1);
    }
    EXPECT_TRUE(waitForStep(2, 100));

    // Prevent the child process from not being completed, the main process ends to clean up resources
    EXPECT_TRUE(waitForStep(1, 100));
}

/**
  * @tc.name: DiffProcessGetAndDeleteDB
  * @tc.desc: Test block other process delete kvstore.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBCommonTest, DiffProcessGetAndDeleteDB, TestSize.Level1)
{
    std::string storeId = "DiffProcessGetAndDeleteDB";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    LOGI("begin fork new process!!");
    pid_t pid = fork();
    ASSERT_TRUE(pid >= 0);
    if (pid == 0) {
        LOGI("child process begin!");
        g_mgr.DeleteKvStore(storeId); // one process OK, one process NOT_FOUND
        createStepFlag(2);
        EXPECT_TRUE(waitForStep(1, 1000));
        exit(0);
    } else {
        LOGI("main process begin!");
        g_mgr.DeleteKvStore(storeId);
        createStepFlag(1);
    }
    EXPECT_TRUE(waitForStep(2, 100));

    // Prevent the child process from not being completed, the main process ends to clean up resources
    EXPECT_TRUE(waitForStep(1, 1000));
}
#endif
#endif

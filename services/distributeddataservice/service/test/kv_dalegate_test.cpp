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
} // namespace OHOS::Test
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
#define LOG_TAG "RdbStoreUtilsTest"

#include "rdb_store_utils.h"
#include "gtest/gtest.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "log_print.h"
#include "result_set.h"
#include "store/general_value.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbStoreUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: RdbStoreUtilsTest001
* @tc.desc: ConvertNativeRdbStatus error code test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbStoreUtilsTest, RdbStoreUtilsTest001, TestSize.Level1)
{
    int32_t status = NativeRdb::E_SQLITE_BUSY;
    int32_t ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_BUSY);
    status = NativeRdb::E_DATABASE_BUSY;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_BUSY);
    status = NativeRdb::E_SQLITE_LOCKED;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_BUSY);
    status = NativeRdb::E_INVALID_ARGS;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    status = NativeRdb::E_INVALID_ARGS_NEW;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    status = NativeRdb::E_ALREADY_CLOSED;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_ALREADY_CLOSED);
    status = NativeRdb::E_SQLITE_CORRUPT;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_DB_CORRUPT);
    status = NativeRdb::E_SQLITE_SCHEMA;
    ret = RdbStoreUtils::ConvertNativeRdbStatus(status);
    EXPECT_EQ(ret, GeneralError::E_ERROR);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
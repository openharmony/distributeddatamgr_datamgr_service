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

#include "rdb_service_stub.h"

#include "gtest/gtest.h"
#include "itypes_util.h"
#include "rdb_service_impl.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: OnStopCloudSync001
* @tc.desc: test stub OnStopCloudSync Unmarshal fail and succ
* @tc.type: FUNC
*/
HWTEST_F(RdbServiceStubTest, OnStopCloudSync001, TestSize.Level1)
{
    auto rdbServiceStub = std::make_shared<RdbServiceImpl>();
    MessageParcel data;
    MessageParcel reply;
    auto ret = rdbServiceStub->OnStopCloudSync(data, reply);
    EXPECT_EQ(ret, IPC_STUB_INVALID_DATA_ERR);
    RdbSyncerParam param;
    ITypesUtil::Marshal(data, param);
    ret = rdbServiceStub->OnStopCloudSync(data, reply);
    EXPECT_EQ(ret, RDB_OK);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
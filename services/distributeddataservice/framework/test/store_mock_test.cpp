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
#define LOG_TAG "StoreMockTest"

#include "access_token.h"
#include "gtest/gtest.h"
#include "general_store_mock.h"
#include "metadata/store_meta_data.h"
#include "rdb_types.h"
#include "store/auto_cache.h"
#include "store/general_watcher.h"
#include "mock/account_delegate_mock.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class AutoCacheMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};


void AutoCacheMockTest::SetUpTestCase(void)
{
}

/**
 * @tc.name: GetDBStoreMockTest001
 * @tc.desc: The test GetDBStoreMock Test
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(AutoCacheMockTest, GetDBStoreMockTest001, TestSize.Level0)
{
    AccountDelegateMock *accountDelegateMock = nullptr;
    accountDelegateMock = new (std::nothrow) AccountDelegateMock();
    ASSERT_NE(accountDelegateMock, nullptr);
    auto creator = [](const StoreMetaData &metaData,
                       const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
        return { GeneralError::E_OK, new (std::nothrow) GeneralStoreMock() };
    };
    AccountDelegate::RegisterAccountInstance(accountDelegateMock);
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);
    AutoCache::Watchers watchers;
    StoreMetaData meta;
    meta.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    meta.area = GeneralStore::EL2;
    meta.user = "100";
    EXPECT_CALL(*accountDelegateMock, IsDeactivating(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*accountDelegateMock, IsVerified(_)).WillRepeatedly(Return(false));
    EXPECT_EQ(AutoCache::GetInstance().GetDBStore(meta, watchers).first, E_USER_LOCKED);

    if (accountDelegateMock != nullptr) {
        delete accountDelegateMock;
        accountDelegateMock = nullptr;
    }
}
} // namespace OHOS::Test
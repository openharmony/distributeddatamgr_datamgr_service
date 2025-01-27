/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "bootstrap.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "types.h"
#include "device_manager_adapter.h"
#include "kvstore_data_service.h"
#include "kvstore_client_death_observer.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/appid_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "feature/feature_system.h"
#include "hap_token_info.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using MetaDataManager = OHOS::DistributedData::MetaDataManager;
using KvStoreDataService = OHOS::DistributedKv::KvStoreDataService;
namespace OHOS::Test {
class KvStoreDataServiceClearTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static constexpr const char *TEST_USER = "100";
    static constexpr const char *TEST_BUNDLE = "ohos.test.demo";
    static constexpr const char *TEST_STORE = "test_store";
    static constexpr int32_t TEST_UID = 2000000;
    static constexpr int32_t TEST_USERID = 100;
    static constexpr const char *BUNDLE_NAME = "ohos.test.demo";
    static constexpr int32_t USER_ID = 100;
    static constexpr int32_t APP_INDEX = 0;
    DistributedData::StoreMetaData metaData_;
    DistributedData::StoreMetaDataLocal localMeta_;
    void InitMetaData();
};

void KvStoreDataServiceClearTest::SetUpTestCase(void)
{
}

void KvStoreDataServiceClearTest::TearDownTestCase(void)
{
}

void KvStoreDataServiceClearTest::SetUp(void)
{
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
}

void KvStoreDataServiceClearTest::TearDown(void)
{
}

/**
 * @tc.name: ClearAppStorage001
 * @tc.desc: Test for abnormal situations in ClearAppStore
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(KvStoreDataServiceClearTest, ClearAppStorage001, TestSize.Level1)
{
    auto kvDataService = OHOS::DistributedKv::KvStoreDataService();
    // tokenId mismatch
    auto ret = kvDataService.ClearAppStorage(BUNDLE_NAME, TEST_USERID, APP_INDEX, 0);
    EXPECT_EQ(ret, Status::ERROR);
    // no meta data
    auto tokenIdOk = Security::AccessToken::AccessTokenKit::GetNativeTokenId("foundation");
    SetSelfTokenID(tokenIdOk);
    ret = kvDataService.ClearAppStorage(BUNDLE_NAME, TEST_USERID, APP_INDEX, tokenIdOk);
    EXPECT_EQ(ret, Status::ERROR);
}
} // namespace OHOS::Test
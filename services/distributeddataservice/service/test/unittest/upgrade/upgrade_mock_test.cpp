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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "upgrade.h"
#include "access_token_mock.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate_mock.h"

using namespace OHOS::DistributedKv;
using namespace testing::ext;
using namespace testing;
using namespace std;
using namespace OHOS::Security::AccessToken;
using namespace DistributedDB;

KvStoreNbDelegateMock g_nbDelegateMock;

DBStatus KvStoreDelegateManager::CloseKvStore(KvStoreDelegate *kvStore)
{
    return DBStatus::NOT_SUPPORT;
}

void KvStoreDelegateManager::GetKvStore(const std::string &storeId, const KvStoreNbDelegate::Option &option,
    const std::function<void(DBStatus, KvStoreNbDelegate *)> &callback)
{
    DBStatus status = DBStatus::OK;
    if (callback) {
        callback(status, &g_nbDelegateMock);
    }
    return;
}

namespace OHOS::Test {
namespace DistributedDataTest {
using DBStatus = DistributedDB::DBStatus;
namespace {
std::string ExporterPwd(const DistributedKv::Upgrade::StoreMeta &storeMeta,
    DistributedKv::Upgrade::DBPassword &pwd)
{
    if (storeMeta.user.empty()) {
        return "";
    }
    return "testIt";
}

Status cleanData(const DistributedKv::Upgrade::StoreMeta &)
{
    return Status::SUCCESS;
}
}

class UpgradeMockTest : public testing::Test {
public:
    using StoreMeta = OHOS::DistributedData::StoreMetaData;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
};

void UpgradeMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
}

void UpgradeMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
}

/**
* @tc.name: UpgradeMockTest001
* @tc.desc: Get encrypted uuid by Meta.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UpgradeMockTest, UpgradeMockTest001, TestSize.Level0)
{
    StoreMeta meta;
    meta.appId = "UpgradeMockTest";
    meta.deviceId = "";
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_INVALID));
    std::string uuid = Upgrade::GetInstance().GetEncryptedUuidByMeta(meta);
    EXPECT_TRUE(uuid.empty());

    meta.appId = "UpgradeTest";
    meta.deviceId = "";
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillRepeatedly(Return(TOKEN_HAP));
    uuid = Upgrade::GetInstance().GetEncryptedUuidByMeta(meta);
    EXPECT_TRUE(uuid.empty());
}

/**
* @tc.name: UpgradeMockTest002
* @tc.desc: Get encrypted uuid by Meta.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UpgradeMockTest, UpgradeMockTest002, TestSize.Level0)
{
    DistributedKv::Upgrade::StoreMeta old;
    old.version = StoreMeta::UUID_CHANGED_TAG + 1,
    old.dataDir = "/data/user/test";
    old.user = "";
    DistributedKv::Upgrade::StoreMeta meta;
    meta.dataDir = "/data/user";
    std::vector<uint8_t> pwd;
    bool ret = Upgrade::GetInstance().RegisterExporter(0, ExporterPwd);
    ret = Upgrade::GetInstance().RegisterCleaner(0, cleanData) && ret;
    EXPECT_TRUE(ret);
    Upgrade::DBStatus status = Upgrade::GetInstance().UpdateStore(old, meta, pwd);
    EXPECT_TRUE(status == Upgrade::DBStatus::NOT_FOUND);
}
}
}
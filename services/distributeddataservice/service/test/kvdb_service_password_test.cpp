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

#include "kvdb_service_impl.h"

#include <gtest/gtest.h>
#include <random>

#include "device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "mock/db_store_mock.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;

static constexpr int32_t KEY_LENGTH = 32;
static constexpr int32_t TEST_USER_NUM = 0;
static constexpr const char *TEST_USER = "0";
static constexpr const char *TEST_APP_ID = "KvdbServicePasswordTest";
static constexpr const char *TEST_STORE_ID = "StoreTest";

class KvdbServicePasswordTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

    static std::vector<uint8_t> Random(int32_t len);

    static AppId appId_;
    static StoreId storeId_;
    static StoreMetaData metaData_;
    static std::shared_ptr<KVDBServiceImpl> kvdbServiceImpl_;
};

AppId KvdbServicePasswordTest::appId_;
StoreId KvdbServicePasswordTest::storeId_;
StoreMetaData KvdbServicePasswordTest::metaData_;
std::shared_ptr<KVDBServiceImpl> KvdbServicePasswordTest::kvdbServiceImpl_;

void KvdbServicePasswordTest::SetUpTestCase(void)
{
    appId_ = { TEST_APP_ID };
    storeId_ = { TEST_STORE_ID };
    kvdbServiceImpl_ = std::make_shared<KVDBServiceImpl>();
    metaData_.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.bundleName = TEST_APP_ID;
    metaData_.appId = TEST_APP_ID;
    metaData_.storeId = TEST_STORE_ID;
    metaData_.user = TEST_USER;
    metaData_.area = Area::EL1;
    metaData_.tokenId = IPCSkeleton::GetSelfTokenID();
}

std::vector<uint8_t> KvdbServicePasswordTest::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

/**
* @tc.name: GetBackupPasswordTest001
* @tc.desc: get backup password with exception branch
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordTest001, TestSize.Level0)
{
    std::vector<std::vector<uint8_t>> passwords;

    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::BACKUP_SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::BUTTON);
    ASSERT_EQ(status, Status::ERROR);

    std::shared_ptr<DBStoreMock> dbStoreMock = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");
    SecretKeyMetaData secretKey;

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    auto key = Random(KEY_LENGTH);
    ASSERT_FALSE(key.empty());
    secretKey.sKey = key;

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordTest002
* @tc.desc: get all type password success
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordTest002, TestSize.Level0)
{
    auto key = Random(KEY_LENGTH);
    ASSERT_FALSE(key.empty());

    std::shared_ptr<DBStoreMock> dbStoreMock = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");

    CryptoManager::CryptoParams encryptParams;
    auto encryptKey = CryptoManager::GetInstance().Encrypt(key, encryptParams);
    ASSERT_FALSE(encryptKey.empty());
    ASSERT_FALSE(encryptParams.nonce.empty());

    SecretKeyMetaData secretKey;
    secretKey.sKey = encryptKey;
    secretKey.nonce = encryptParams.nonce;
    std::vector<std::vector<uint8_t>> passwords;

    // 1.get backup secret key success
    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::BACKUP_SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    ASSERT_EQ(passwords[0].size(), key.size());
    for (auto i = 0; i < key.size(); ++i) {
        ASSERT_EQ(passwords[0][i], key[i]);
    }
    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);

    // 2.get secret key success
    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    ASSERT_EQ(passwords[0].size(), key.size());
    for (auto i = 0; i < key.size(); ++i) {
        ASSERT_EQ(passwords[0][i], key[i]);
    }
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);

    // 3.get clone secret key success
    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, TEST_USER_NUM, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    ASSERT_EQ(passwords[0].size(), key.size());
    for (auto i = 0; i < key.size(); ++i) {
        ASSERT_EQ(passwords[0][i], key[i]);
    }
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}
} // namespace OHOS::Test
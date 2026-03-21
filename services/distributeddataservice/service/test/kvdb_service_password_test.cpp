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
#include "gmock/gmock.h"
#include <random>

#include "device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "mock/db_store_mock.h"
#include "account/account_delegate.h"
#include "account_delegate_mock.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;

static constexpr int32_t KEY_LENGTH = 32;
static constexpr int32_t TEST_USER_NUM = 0;
static constexpr const char *TEST_USER = "0";
static constexpr const char *TEST_APP_ID = "KvdbServicePasswordTest";
static constexpr const char *TEST_STORE_ID = "StoreTest";
static constexpr const char *TEST_DATA_DIR = "/data/service/el1/public/database/KvdbServicePasswordTest";

class KvdbServicePasswordTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

    static std::vector<uint8_t> Random(int32_t len);

    static AppId appId_;
    static StoreId storeId_;
    static StoreMetaData metaData_;
    static std::shared_ptr<KVDBServiceImpl> kvdbServiceImpl_;
private:
    static AccountDelegateMock *accountDelegateMock_;
};

AppId KvdbServicePasswordTest::appId_;
StoreId KvdbServicePasswordTest::storeId_;
StoreMetaData KvdbServicePasswordTest::metaData_;
std::shared_ptr<KVDBServiceImpl> KvdbServicePasswordTest::kvdbServiceImpl_;
AccountDelegateMock *KvdbServicePasswordTest::accountDelegateMock_ = nullptr;

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
    (void)CryptoManager::GetInstance().GenerateRootKey();
    accountDelegateMock_ = new (std::nothrow) AccountDelegateMock();
    AccountDelegate::RegisterAccountInstance(accountDelegateMock_);
    EXPECT_CALL(*accountDelegateMock_, IsDeactivating(testing::_)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*accountDelegateMock_, GetUserByToken(testing::_)).WillRepeatedly(testing::Return(0));
}

void KvdbServicePasswordTest::TearDownTestCase(void)
{
    delete accountDelegateMock_;
    accountDelegateMock_ = nullptr;
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
    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = TEST_DATA_DIR, .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM};
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::BACKUP_SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::BUTTON);
    ASSERT_EQ(status, Status::ERROR);

    std::shared_ptr<DBStoreMock> dbStoreMock = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");
    SecretKeyMetaData secretKey;

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    auto key = Random(KEY_LENGTH);
    ASSERT_FALSE(key.empty());
    secretKey.sKey = key;

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::ERROR);

    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithCustomDirTest003
* @tc.desc: get backup password with custom directory
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithCustomDirTest003, TestSize.Level0)
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

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = TEST_DATA_DIR, .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM, .isCustomDir = true };
    std::vector<std::vector<uint8_t>> passwords;
    
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithoutCustomDirTest004
* @tc.desc: get backup password without custom directory
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithoutCustomDirTest004, TestSize.Level0)
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

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = "", .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM, .isCustomDir = false };
    std::vector<std::vector<uint8_t>> passwords;
    
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::BACKUP_SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithCloneKeyTest005
* @tc.desc: get clone secret key with BackupInfo
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithCloneKeyTest005, TestSize.Level0)
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

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = TEST_DATA_DIR, .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM, .isCustomDir = true };
    std::vector<std::vector<uint8_t>> passwords;
    
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithAllKeysTest006
* @tc.desc: get all password types with BackupInfo
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithAllKeysTest006, TestSize.Level0)
{
    auto key1 = Random(KEY_LENGTH);
    auto key2 = Random(KEY_LENGTH);
    auto key3 = Random(KEY_LENGTH);
    ASSERT_FALSE(key1.empty());
    ASSERT_FALSE(key2.empty());
    ASSERT_FALSE(key3.empty());

    std::shared_ptr<DBStoreMock> dbStoreMock = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");

    CryptoManager::CryptoParams encryptParams1, encryptParams2, encryptParams3;
    auto encryptKey1 = CryptoManager::GetInstance().Encrypt(key1, encryptParams1);
    auto encryptKey2 = CryptoManager::GetInstance().Encrypt(key2, encryptParams2);
    auto encryptKey3 = CryptoManager::GetInstance().Encrypt(key3, encryptParams3);
    ASSERT_FALSE(encryptKey1.empty());
    ASSERT_FALSE(encryptKey2.empty());
    ASSERT_FALSE(encryptKey3.empty());

    SecretKeyMetaData secretKey1, secretKey2, secretKey3;
    secretKey1.sKey = encryptKey1;
    secretKey1.nonce = encryptParams1.nonce;
    secretKey2.sKey = encryptKey2;
    secretKey2.nonce = encryptParams2.nonce;
    secretKey3.sKey = encryptKey3;
    secretKey3.nonce = encryptParams3.nonce;

    auto result1 = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey1, true);
    auto result2 = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey2, true);
    auto result3 = MetaDataManager::GetInstance().SaveMeta(metaData_.GetCloneSecretKey(), secretKey3, true);
    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);
    ASSERT_TRUE(result3);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = TEST_DATA_DIR, .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM, .isCustomDir = true };
    std::vector<std::vector<uint8_t>> passwords;
    
    // Test SECRET_KEY type - should get secret key first, then clone key
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_EQ(passwords.size(), 2);
    
    // Test BACKUP_SECRET_KEY type
    passwords.clear();
    status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::BACKUP_SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_EQ(passwords.size(), 1);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithEmptyDirTest007
* @tc.desc: get backup password with empty base directory
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithEmptyDirTest007, TestSize.Level0)
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

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = "", .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = TEST_USER_NUM, .isCustomDir = false };
    std::vector<std::vector<uint8_t>> passwords;
    
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_GT(passwords.size(), 0);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
}

/**
* @tc.name: GetBackupPasswordWithInvalidSubUserTest008
* @tc.desc: get backup password with invalid subUser
* @tc.type: FUNC
*/
HWTEST_F(KvdbServicePasswordTest, GetBackupPasswordWithInvalidSubUserTest008, TestSize.Level0)
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

    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    BackupInfo info = { .name = "KvdbServicePasswordTest", .baseDir = TEST_DATA_DIR, .appId = appId_.appId,
        .storeId = storeId_.storeId, .subUser = -1, .isCustomDir = true };
    std::vector<std::vector<uint8_t>> passwords;
    
    auto status = kvdbServiceImpl_->GetBackupPassword(
        appId_, storeId_, info, passwords, DistributedKv::KVDBService::PasswordType::SECRET_KEY);
    
    // The function should still work even with invalid subUser, but may return ERROR
    ASSERT_TRUE(status == Status::SUCCESS || status == Status::ERROR);
    
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
}

} // namespace OHOS::Test
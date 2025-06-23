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
#define LOG_TAG "BackupManagerServiceTest"

#include <random>
#include <thread>
#include <gtest/gtest.h>
#include "backup_manager.h"
#include "backuprule/backup_rule_manager.h"
#include "bootstrap.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory/directory_manager.h"
#include "file_ex.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "kvstore_meta_manager.h"
#include "types.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr int32_t LOOP_NUM = 2;
static constexpr uint32_t SKEY_SIZE = 32;
static constexpr int MICROSEC_TO_SEC_TEST = 1000;
static constexpr const char *TEST_BACKUP_BUNDLE = "test_backup_bundleName";
static constexpr const char *TEST_BACKUP_STOREID = "test_backup_storeId";
static constexpr const char *BASE_DIR = "/data/service/el1/public/database/test_backup_bundleName";
static constexpr const char *DATA_DIR = "/data/service/el1/public/database/test_backup_bundleName/rdb";
static constexpr const char *BACKUP_DIR = "/data/service/el1/public/database/test_backup_bundleName/rdb/backup";
static constexpr const char *STORE_DIR =
    "/data/service/el1/public/database/test_backup_bundleName/rdb/backup/test_backup_storeId";
static constexpr const char *AUTO_BACKUP_NAME = "/autoBackup.bak";
class BackupManagerServiceTest : public testing::Test {
public:
    class TestRule : public BackupRuleManager::BackupRule {
    public:
        TestRule()
        {
            BackupRuleManager::GetInstance().RegisterPlugin("TestRule", [this]() -> auto {
                return this;
            });
        }
        bool CanBackup() override
        {
            return false;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static std::vector<uint8_t> Random(uint32_t len);
    static void InitMetaData();
    static void Exporter(const StoreMetaData &meta, const std::string &backupPath, bool &result);
    static void ConfigExport(bool flag);

    static StoreMetaData metaData_;
    static bool isExport_;
};

StoreMetaData BackupManagerServiceTest::metaData_;
bool BackupManagerServiceTest::isExport_ = false;

void BackupManagerServiceTest::SetUpTestCase(void)
{
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    DeviceManagerAdapter::GetInstance().Init(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    InitMetaData();
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
}

void BackupManagerServiceTest::TearDownTestCase()
{
}

void BackupManagerServiceTest::SetUp()
{
}

void BackupManagerServiceTest::TearDown()
{
}

void BackupManagerServiceTest::InitMetaData()
{
    metaData_.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.bundleName = TEST_BACKUP_BUNDLE;
    metaData_.appId = TEST_BACKUP_BUNDLE;
    metaData_.user = "0";
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = StoreMetaData::StoreType::STORE_KV_BEGIN;
    metaData_.storeId = TEST_BACKUP_STOREID;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(TEST_BACKUP_BUNDLE) + "/kvdb";
    metaData_.securityLevel = OHOS::DistributedKv::SecurityLevel::S2;
    metaData_.tokenId = IPCSkeleton::GetSelfTokenID();
}

std::vector<uint8_t> BackupManagerServiceTest::Random(uint32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

void BackupManagerServiceTest::ConfigExport(bool flag)
{
    isExport_ = flag;
}

void BackupManagerServiceTest::Exporter(const StoreMetaData &meta, const std::string &backupPath, bool &result)
{
    (void)meta;
    result = isExport_;
    if (isExport_) {
        std::vector<char> content(TEST_BACKUP_BUNDLE, TEST_BACKUP_BUNDLE + std::strlen(TEST_BACKUP_BUNDLE));
        result = SaveBufferToFile(backupPath, content);
    }
}

/**
* @tc.name: Init
* @tc.desc: Init testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, Init, TestSize.Level1)
{
    BackupManagerServiceTest::InitMetaData();
    StoreMetaData meta1;
    meta1 = metaData_;
    meta1.isBackup = false;
    meta1.isDirty = false;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid }), meta1, true));
    BackupManager::GetInstance().Init();

    StoreMetaData meta2;
    meta2 = metaData_;
    meta2.isBackup = true;
    meta2.isDirty = false;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid }), meta2, true));
    BackupManager::GetInstance().Init();

    StoreMetaData meta3;
    meta3 = metaData_;
    meta3.isBackup = false;
    meta3.isDirty = true;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid }), meta3, true));
    BackupManager::GetInstance().Init();

    StoreMetaData meta4;
    meta4 = metaData_;
    meta4.isBackup = true;
    meta4.isDirty = true;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid }), meta4, true));
    BackupManager::GetInstance().Init();
}

/**
* @tc.name: RegisterExporter
* @tc.desc: RegisterExporter testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, RegisterExporter, TestSize.Level1)
{
    int32_t type = DistributedKv::KvStoreType::DEVICE_COLLABORATION;
    BackupManager::Exporter exporter =
        [](const StoreMetaData &meta, const std::string &backupPath, bool &result)
        { result = true; };
    BackupManager instance;
    instance.RegisterExporter(type, exporter);
    EXPECT_FALSE(instance.exporters_[type] == nullptr);
    instance.RegisterExporter(type, exporter);
}

/**
* @tc.name: BackSchedule
* @tc.desc: BackSchedule testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, BackSchedule, TestSize.Level1)
{
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(0, 1);
    BackupManager instance;
    instance.executors_ = nullptr;
    instance.BackSchedule(executors);
    EXPECT_FALSE(instance.executors_ == nullptr);
}

/**
* @tc.name: CanBackup001
* @tc.desc: CanBackup testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, CanBackup001, TestSize.Level1)
{
    BackupManager instance;
    instance.backupSuccessTime_ = 0;
    instance.backupInternal_ = 0;
    bool status = instance.CanBackup(); // false false
    EXPECT_TRUE(status);

    instance.backupInternal_ = MICROSEC_TO_SEC_TEST;
    status = instance.CanBackup(); // true false
    EXPECT_TRUE(status);

    instance.backupSuccessTime_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    instance.backupInternal_ = 0;
    status = instance.CanBackup(); // false true
    EXPECT_TRUE(status);

    instance.backupSuccessTime_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    instance.backupInternal_ = MICROSEC_TO_SEC_TEST;
    status = instance.CanBackup(); // true true
    EXPECT_FALSE(status);
}

/**
* @tc.name: CanBackup
* @tc.desc: CanBackup testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, CanBackup, TestSize.Level1)
{
    BackupManagerServiceTest::TestRule();
    std::vector<std::string> rule = { "TestRule" };
    BackupRuleManager::GetInstance().LoadBackupRules(rule);
    EXPECT_FALSE(BackupRuleManager::GetInstance().CanBackup());
    bool status = BackupManager::GetInstance().CanBackup();
    EXPECT_FALSE(status);
}

/**
* @tc.name: SaveData
* @tc.desc: SaveData testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, SaveData, TestSize.Level1)
{
    std::string path = "test";
    std::string key = "test";
    std::vector<uint8_t> randomKey = BackupManagerServiceTest::Random(SKEY_SIZE);
    SecretKeyMetaData secretKey;
    secretKey.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    secretKey.sKey = randomKey;
    EXPECT_EQ(secretKey.sKey.size(), SKEY_SIZE);
    BackupManager::GetInstance().SaveData(path, key, secretKey);
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(key, secretKey, true));
    randomKey.assign(randomKey.size(), 0);
}

/**
* @tc.name: GetClearType001
* @tc.desc: GetClearType testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, GetClearType001, TestSize.Level1)
{
    StoreMetaData meta;
    MetaDataManager::GetInstance().SaveMeta(StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid }), metaData_, true);
    BackupManager::ClearType status = BackupManager::GetInstance().GetClearType(meta);
    EXPECT_EQ(status, BackupManager::ClearType::DO_NOTHING);
}

/**
* @tc.name: GetClearType002
* @tc.desc: GetClearType testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, GetClearType002, TestSize.Level1)
{
    BackupManagerServiceTest::InitMetaData();
    StoreMetaData meta;
    meta = metaData_;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), meta, true));

    SecretKeyMetaData dbPassword;
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(meta.GetSecretKey(), dbPassword, true));
    SecretKeyMetaData backupPassword;
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(meta.GetBackupSecretKey(), backupPassword, true));
    EXPECT_FALSE(dbPassword.sKey != backupPassword.sKey);
    BackupManager::ClearType status = BackupManager::GetInstance().GetClearType(meta);
    EXPECT_EQ(status, BackupManager::ClearType::DO_NOTHING);
}

/**
* @tc.name: GetPassWordTest001
* @tc.desc: get password fail with exception branch
* @tc.type: FUNC
*/
HWTEST_F(BackupManagerServiceTest, GetPassWordTest001, TestSize.Level1)
{
    StoreMetaData meta;
    auto password = BackupManager::GetInstance().GetPassWord(meta);
    ASSERT_TRUE(password.empty());

    SecretKeyMetaData secretKey;
    secretKey.area = metaData_.area;
    secretKey.storeType = metaData_.storeType;
    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    password = BackupManager::GetInstance().GetPassWord(metaData_);
    ASSERT_TRUE(password.empty());

    auto key = Random(SKEY_SIZE);
    ASSERT_FALSE(key.empty());
    secretKey.sKey = key;
    secretKey.nonce = key;
    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    password = BackupManager::GetInstance().GetPassWord(metaData_);
    ASSERT_TRUE(password.empty());

    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
}

/**
* @tc.name: GetPassWordTest002
* @tc.desc: get backup password success
* @tc.type: FUNC
*/
HWTEST_F(BackupManagerServiceTest, GetPassWordTest002, TestSize.Level1)
{
    auto key = Random(SKEY_SIZE);
    ASSERT_FALSE(key.empty());
    CryptoManager::CryptoParams encryptParams;
    auto encryptKey = CryptoManager::GetInstance().Encrypt(key, encryptParams);
    ASSERT_FALSE(encryptKey.empty());

    SecretKeyMetaData secretKey;
    secretKey.area = encryptParams.area;
    secretKey.storeType = metaData_.storeType;
    secretKey.sKey = encryptKey;
    secretKey.nonce = encryptParams.nonce;
    auto result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetBackupSecretKey(), secretKey, true);
    ASSERT_TRUE(result);

    for (int32_t index = 0; index < LOOP_NUM; ++index) {
        auto password = BackupManager::GetInstance().GetPassWord(metaData_);
        ASSERT_FALSE(password.empty());
        ASSERT_EQ(password.size(), key.size());
        for (size_t i = 0; i < key.size(); ++i) {
            ASSERT_EQ(password[i], key[i]);
        }
    }

    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
}

/**
* @tc.name: DoBackupTest001
* @tc.desc: do backup with every condition
* @tc.type: FUNC
*/
HWTEST_F(BackupManagerServiceTest, DoBackupTest001, TestSize.Level1)
{
    metaData_.storeType = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    mkdir(BASE_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir(DATA_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir(BACKUP_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    mkdir(STORE_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    auto backupPath = DirectoryManager::GetInstance().GetStoreBackupPath(metaData_);
    ASSERT_EQ(backupPath, STORE_DIR);

    std::string backupFilePath = backupPath + AUTO_BACKUP_NAME;

    std::shared_ptr<BackupManager> testManager = std::make_shared<BackupManager>();
    testManager->DoBackup(metaData_);
    ASSERT_NE(access(backupFilePath.c_str(), F_OK), 0);

    testManager->RegisterExporter(metaData_.storeType, Exporter);
    ConfigExport(false);
    testManager->DoBackup(metaData_);
    ASSERT_NE(access(backupFilePath.c_str(), F_OK), 0);

    ConfigExport(true);
    metaData_.isEncrypt = false;
    testManager->DoBackup(metaData_);
    ASSERT_EQ(access(backupFilePath.c_str(), F_OK), 0);
    (void)remove(backupFilePath.c_str());

    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    SecretKeyMetaData secretKey;
    auto result = MetaDataManager::GetInstance().LoadMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_FALSE(result);

    metaData_.isEncrypt = true;
    testManager->DoBackup(metaData_);
    ASSERT_NE(access(backupFilePath.c_str(), F_OK), 0);

    SecretKeyMetaData backupSecretKey;
    result = MetaDataManager::GetInstance().LoadMeta(metaData_.GetBackupSecretKey(), backupSecretKey, true);
    ASSERT_FALSE(result);

    secretKey.area = metaData_.area;
    secretKey.storeType = metaData_.storeType;
    auto key = Random(SKEY_SIZE);
    ASSERT_FALSE(key.empty());
    secretKey.sKey = key;
    result = MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true);
    ASSERT_TRUE(result);
    testManager->DoBackup(metaData_);
    ASSERT_EQ(access(backupFilePath.c_str(), F_OK), 0);
    result = MetaDataManager::GetInstance().LoadMeta(metaData_.GetBackupSecretKey(), backupSecretKey, true);
    ASSERT_TRUE(result);

    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetBackupSecretKey(), true);
    (void)remove(backupFilePath.c_str());
    (void)remove(STORE_DIR);
    (void)remove(BACKUP_DIR);
    (void)remove(DATA_DIR);
    (void)remove(BASE_DIR);
}

/**
* @tc.name: IsFileExist
* @tc.desc: IsFileExist testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, IsFileExist, TestSize.Level1)
{
    std::string path;
    bool status = BackupManager::GetInstance().IsFileExist(path);
    EXPECT_FALSE(status);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
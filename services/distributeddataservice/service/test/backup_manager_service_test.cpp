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
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "kvstore_meta_manager.h"
#include "types.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr uint32_t SKEY_SIZE = 32;
static constexpr int MICROSEC_TO_SEC_TEST = 1000;
static constexpr const char *TEST_BACKUP_BUNDLE = "test_backup_bundleName";
static constexpr const char *TEST_BACKUP_STOREID = "test_backup_storeId";
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
    void InitMetaData();
    StoreMetaData metaData_;
};

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
    metaData_.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData_.storeId = TEST_BACKUP_STOREID;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(TEST_BACKUP_BUNDLE) + "/kvdb";
    metaData_.securityLevel = OHOS::DistributedKv::SecurityLevel::S2;
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
    EXPECT_NO_FATAL_FAILURE(instance.RegisterExporter(type, exporter));
    EXPECT_FALSE(instance.exporters_[type] == nullptr);
    EXPECT_NO_FATAL_FAILURE(instance.RegisterExporter(type, exporter));
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
    EXPECT_NO_FATAL_FAILURE(instance.BackSchedule(executors));
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
    ASSERT_FALSE(BackupRuleManager::GetInstance().CanBackup());
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
    EXPECT_NO_FATAL_FAILURE(BackupManager::GetInstance().SaveData(path, key, secretKey));
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
* @tc.name: GetPassWord
* @tc.desc: GetPassWord testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(BackupManagerServiceTest, GetPassWord, TestSize.Level1)
{
    StoreMetaData meta;
    std::vector<uint8_t> password;
    bool status = BackupManager::GetInstance().GetPassWord(meta, password);
    EXPECT_FALSE(status);
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
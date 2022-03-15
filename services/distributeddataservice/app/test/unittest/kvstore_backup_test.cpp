/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <memory>
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "kvstore_impl.h"
#include "backup_handler.h"
#include "kv_scheduler.h"
#include "kvstore_data_service.h"
#include "kvstore_meta_manager.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "single_kvstore_impl.h"
#include "gtest/gtest.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using namespace OHOS::DistributedData;

class KvStoreBackupTest : public testing::Test {
public:
    static constexpr unsigned int DEFAULT_DIR_MODE = 0755;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvStoreBackupTest::SetUpTestCase(void)
{}

void KvStoreBackupTest::TearDownTestCase(void)
{}

void KvStoreBackupTest::SetUp(void)
{
    const std::string backupDirCe = "/data/misc_ce/0/mdds/0/default/backup";
    unlink(backupDirCe.c_str());
    mkdir(backupDirCe.c_str(), KvStoreBackupTest::DEFAULT_DIR_MODE);

    const std::string backupDirDe = "/data/misc_de/0/mdds/0/default/backup";
    unlink(backupDirDe.c_str());
    mkdir(backupDirDe.c_str(), KvStoreBackupTest::DEFAULT_DIR_MODE);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
}

void KvStoreBackupTest::TearDown(void)
{}

/**
* @tc.name: KvStoreBackupTest001
* @tc.desc: set option backup
* @tc.type: FUNC
* @tc.require: SR000DR9J0 AR000DR9J1
* @tc.author: guodaoxin
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest001, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup1" };
    StoreId storeId = { "store1" };
    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest001 set backup true failed";
    kvDataService.CloseKvStore(appId, storeId);
}

/**
* @tc.name: KvStoreBackupTest002
* @tc.desc: kvstore backup test for single db
* @tc.type: FUNC
* @tc.require: SR000DR9J0 AR000DR9JM
* @tc.author: guodaoxin
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest002, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup2" };
    StoreId storeId = { "store2" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest002 set backup true failed";
    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreType = KvStoreType::SINGLE_VERSION;

    backupHandler->SingleKvStoreBackup(metaData);

    kvStorePtr->Delete(key2);
    Value value22;
    kvStorePtr->Get(key2, value22);
    auto kptr = static_cast<SingleKvStoreImpl *>(kvStorePtr.GetRefPtr());
    bool importRes = kptr->Import(appId.appId);
    EXPECT_EQ(importRes, true) << "KvStoreBackupTest002 NO_LABEL single kvstore import failed";
    kvStorePtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest002 single kvstore backup failed";

    kvDataService.CloseKvStore(appId, storeId);
}
/**
* @tc.name: KvStoreBackupTest003
* @tc.desc: kvstore backup test for multi db
* @tc.type: FUNC
* @tc.require: AR000DR9JM AR000D08K5
* @tc.author: guodaoxin
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest003, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .kvStoreType = KvStoreType::MULTI_VERSION, .dataOwnership = true };
    AppId appId = { "backup3" };
    StoreId storeId = { "store3" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<IKvStoreImpl> kvStorePtr;
    Status status = kvDataService.GetKvStore(options, appId, storeId,
        [&](sptr<IKvStoreImpl> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest003 set backup true failed";

    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreType = KvStoreType::MULTI_VERSION;

    backupHandler->MultiKvStoreBackup(metaData);

    kvStorePtr->Delete(key2);

    auto kptr = static_cast<KvStoreImpl *>(kvStorePtr.GetRefPtr());
    kptr->Import(appId.appId);

    sptr<IKvStoreSnapshotImpl> kvStoreSnapshotPtr;
    kvStorePtr->GetKvStoreSnapshot(nullptr,
                                   [&](Status status, sptr<IKvStoreSnapshotImpl> kvStoreSnapshot) {
                                       kvStoreSnapshotPtr = std::move(kvStoreSnapshot);
                                   });

    EXPECT_NE(nullptr, kvStoreSnapshotPtr) << "KvStoreBackupTest003, kvStoreSnapshotPtr is nullptr";

    Value value22;
    kvStoreSnapshotPtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest003 muti kvstore backup failed";

    kvStorePtr->ReleaseKvStoreSnapshot(std::move(kvStoreSnapshotPtr));
    kvDataService.CloseKvStore(appId, storeId);
}
/**
* @tc.name: KvStoreBackupTest004
* @tc.desc: kvstore backup delete test
* @tc.type: FUNC
* @tc.require:AR000G2VNB
* @tc.author:zuojiangjiang
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest004, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
            .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup4" };
    StoreId storeId = { "store4" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest004 set backup true failed";

    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreType = KvStoreType::SINGLE_VERSION;

    backupHandler->SingleKvStoreBackup(metaData);

    auto currentAccountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    std::initializer_list<std::string> fileList = {currentAccountId, "_", trueAppId, "_", storeId.storeId};
    auto backupFileName = Constant::Concatenate(fileList);
    auto backupFileNameHashed = BackupHandler::GetHashedBackupName(backupFileName);
    auto pathType = KvStoreAppManager::ConvertPathType(1000, appId.appId, metaData.kvStoreMetaData.securityLevel);
    std::initializer_list<std::string> backFileList = {BackupHandler::GetBackupPath("0", pathType),
        "/", backupFileNameHashed};
    auto backFilePath = Constant::Concatenate(backFileList);
    bool ret = BackupHandler::FileExists(backFilePath);
    EXPECT_EQ(ret, true) << "KvStoreBackupTest004 backup file failed";

    kvDataService.CloseKvStore(appId, storeId);
    kvDataService.DeleteKvStore(appId, storeId);
    ret = BackupHandler::FileExists(backFilePath);
    EXPECT_EQ(ret, false) << "KvStoreBackupTest004 delete backup file failed";
}
/**
* @tc.name: KvStoreBackupTest005
* @tc.desc: S0 kvstore backup test for single db
* @tc.type: FUNC
* @tc.require:AR000G2VNB
* @tc.author:zuojiangjiang
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest005, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .securityLevel = SecurityLevel::S0, .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup5" };
    StoreId storeId = { "store5" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest005 set backup true failed";
    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreMetaData.securityLevel = SecurityLevel::S0;
    metaData.kvStoreType = KvStoreType::SINGLE_VERSION;

    backupHandler->SingleKvStoreBackup(metaData);

    kvStorePtr->Delete(key2);
    Value value22;
    kvStorePtr->Get(key2, value22);
    auto kptr = static_cast<SingleKvStoreImpl *>(kvStorePtr.GetRefPtr());
    bool importRes = kptr->Import(appId.appId);
    EXPECT_EQ(importRes, true) << "KvStoreBackupTest005 S0 single kvstore import failed";
    kvStorePtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest005 S0 single kvstore backup failed";

    kvDataService.CloseKvStore(appId, storeId);
}
/**
* @tc.name: KvStoreBackupTest006
* @tc.desc: S2 kvstore backup test for single db
* @tc.type: FUNC
* @tc.require:AR000G2VNB
* @tc.author:zuojiangjiang
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest006, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .securityLevel = SecurityLevel::S2, .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup6" };
    StoreId storeId = { "store6" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest006 set backup true failed";
    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreMetaData.securityLevel = SecurityLevel::S2;
    metaData.kvStoreType = KvStoreType::SINGLE_VERSION;

    backupHandler->SingleKvStoreBackup(metaData);

    kvStorePtr->Delete(key2);
    Value value22;
    kvStorePtr->Get(key2, value22);
    auto kptr = static_cast<SingleKvStoreImpl *>(kvStorePtr.GetRefPtr());
    bool importRes = kptr->Import(appId.appId);
    EXPECT_EQ(importRes, true) << "KvStoreBackupTest006 S2 single kvstore import failed";
    kvStorePtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest006 S2 single kvstore backup failed";

    kvDataService.CloseKvStore(appId, storeId);
}
/**
* @tc.name: KvStoreBackupTest007
* @tc.desc: S4 kvstore backup test for single db
* @tc.type: FUNC
* @tc.require:AR000G2VNB
* @tc.author:zuojiangjiang
*/
HWTEST_F(KvStoreBackupTest, KvStoreBackupTest007, TestSize.Level1)
{
    Options options = { .createIfMissing = true, .encrypt = false, .backup = true, .autoSync = true,
        .securityLevel = SecurityLevel::S4, .kvStoreType = KvStoreType::SINGLE_VERSION, .dataOwnership = true };
    AppId appId = { "backup7" };
    StoreId storeId = { "store7" };

    KvStoreDataService kvDataService;
    kvDataService.DeleteKvStore(appId, storeId);
    sptr<ISingleKvStore> kvStorePtr;
    Status status = kvDataService.GetSingleKvStore(options, appId, storeId,
        [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore);});

    EXPECT_EQ(status, Status::SUCCESS) << "KvStoreBackupTest007 set backup true failed";
    Key key1("test1_key");
    Value value1("test1_value");
    kvStorePtr->Put(key1, value1);
    Key key2("test2_key");
    Value value2("test2_value");
    kvStorePtr->Put(key2, value2);

    auto trueAppId = CheckerManager::GetInstance().GetAppId(appId.appId, 1000);
    auto backupHandler = std::make_unique<BackupHandler>();
    MetaData metaData;
    metaData.kvStoreMetaData.deviceAccountId = "0";
    metaData.kvStoreMetaData.userId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.kvStoreMetaData.appId = trueAppId;
    metaData.kvStoreMetaData.bundleName = appId.appId;
    metaData.kvStoreMetaData.storeId = storeId.storeId;
    metaData.kvStoreMetaData.isBackup = true;
    metaData.kvStoreMetaData.securityLevel = SecurityLevel::S4;
    metaData.kvStoreType = KvStoreType::SINGLE_VERSION;

    backupHandler->SingleKvStoreBackup(metaData);

    kvStorePtr->Delete(key2);
    Value value22;
    kvStorePtr->Get(key2, value22);
    auto kptr = static_cast<SingleKvStoreImpl *>(kvStorePtr.GetRefPtr());
    bool importRes = kptr->Import(appId.appId);
    EXPECT_EQ(importRes, true) << "KvStoreBackupTest007 S4 single kvstore import failed";
    kvStorePtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest007 S0 single kvstore backup failed";

    kvDataService.CloseKvStore(appId, storeId);
}
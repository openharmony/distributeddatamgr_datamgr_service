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
#include <gtest/gtest.h>
#include <unistd.h>
#include <memory>
#include <thread>
#include <vector>

#include "backup_handler.h"
#include "bootstrap.h"
#include "communication_provider.h"
#include "kv_store_delegate_manager.h"
#include "kvstore_data_service.h"
#include "metadata/meta_data_manager.h"
#include "process_communicator_impl.h"
#include "session_manager/route_head_handler_impl.h"
#include "single_kvstore_impl.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;

class KvStoreBackupTest : public testing::Test {
public:
    static constexpr unsigned int DEFAULT_DIR_MODE = 0755;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    std::string deviceId_;
};

void KvStoreBackupTest::SetUpTestCase(void)
{
    const std::string backupDirCe = "/data/misc_ce/0/mdds/0/default/backup";
    unlink(backupDirCe.c_str());
    mkdir(backupDirCe.c_str(), KvStoreBackupTest::DEFAULT_DIR_MODE);

    const std::string backupDirDe = "/data/misc_de/0/mdds/0/default/backup";
    unlink(backupDirDe.c_str());
    mkdir(backupDirDe.c_str(), KvStoreBackupTest::DEFAULT_DIR_MODE);
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    KvStoreMetaManager::GetInstance().InitMetaListener();
    DistributedDB::KvStoreDelegateManager::SetProcessLabel("KvStoreBackupTest", "default");
    auto communicator = std::make_shared<AppDistributedKv::ProcessCommunicatorImpl>(RouteHeadHandlerImpl::Create);
    (void)DistributedDB::KvStoreDelegateManager::SetProcessCommunicator(communicator);
}

void KvStoreBackupTest::TearDownTestCase(void)
{}

void KvStoreBackupTest::SetUp(void)
{
    deviceId_ = CommunicationProvider::GetInstance().GetLocalDevice().uuid;
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
    Options options = { .kvStoreType = KvStoreType::SINGLE_VERSION};
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
    Options options = { .kvStoreType = KvStoreType::SINGLE_VERSION };
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

    auto backupHandler = std::make_unique<BackupHandler>();
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.deviceId = deviceId_;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    backupHandler->DoBackup(metaData);

    kvStorePtr->Delete(key2);
    Value value22;
    kvStorePtr->Get(key2, value22);
    auto kptr = static_cast<SingleKvStoreImpl *>(kvStorePtr.GetRefPtr());
    bool importRes = kptr->Import(appId.appId);
    EXPECT_TRUE(importRes) << "KvStoreBackupTest002 NO_LABEL single kvstore import failed";
    kvStorePtr->Get(key2, value22);
    EXPECT_EQ(value22.ToString(), value2.ToString()) << "KvStoreBackupTest002 single kvstore backup failed";
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
    Options options = { .kvStoreType = KvStoreType::SINGLE_VERSION};
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

    auto backupHandler = std::make_unique<BackupHandler>();
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.deviceId = deviceId_;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    backupHandler->DoBackup(metaData);

    auto currentAccountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    std::initializer_list<std::string> fileList = {currentAccountId, "_", metaData.appId, "_", storeId.storeId};
    auto backupFileName = Constant::Concatenate(fileList);
    auto backupFileNameHashed = BackupHandler::GetHashedBackupName(backupFileName);
    auto pathType = KvStoreAppManager::ConvertPathType(metaData);
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
    Options options = { .securityLevel = SecurityLevel::S0, .kvStoreType = KvStoreType::SINGLE_VERSION};
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

    auto backupHandler = std::make_unique<BackupHandler>();
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.deviceId = deviceId_;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    backupHandler->DoBackup(metaData);

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
    Options options = { .securityLevel = SecurityLevel::S2, .kvStoreType = KvStoreType::SINGLE_VERSION};
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

    auto backupHandler = std::make_unique<BackupHandler>();
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.deviceId = deviceId_;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    backupHandler->DoBackup(metaData);

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
    Options options = { .securityLevel = SecurityLevel::S4, .kvStoreType = KvStoreType::SINGLE_VERSION};
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

    auto backupHandler = std::make_unique<BackupHandler>();
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.deviceId = deviceId_;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    backupHandler->DoBackup(metaData);

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
/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "utils/constant.h"
#include <nlohmann/json.hpp>
#include "bootstrap.h"
#include "kvstore_meta_manager.h"
#include "metadata/appid_meta_data.h"
#include "metadata/corrupted_meta_data.h"
#include "metadata/capability_range.h"
#include "metadata/meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/strategy_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/user_meta_data.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class ServiceMetaDataTest : public testing::Test {
public:
    static constexpr size_t NUM_MIN = 1;
    static constexpr size_t NUM_MAX = 2;
    static constexpr uint32_t USER_ID1 = 101;
    static constexpr uint32_t USER_ID2 = 100;
    static constexpr uint32_t TEST_CURRENT_VERSION = 0x03000002;
    static void SetUpTestCase()
    {
        std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        Bootstrap::GetInstance().LoadComponents();
        Bootstrap::GetInstance().LoadDirectory();
        Bootstrap::GetInstance().LoadCheckers();
        KvStoreMetaManager::GetInstance().BindExecutor(executors);
        KvStoreMetaManager::GetInstance().InitMetaParameter();
        KvStoreMetaManager::GetInstance().InitMetaListener();
    }
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
* @tc.name: AppIDMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, AppIDMetaData, TestSize.Level1)
{
    AppIDMetaData appIdMetaData("appid", "ohos.test.demo");
    AppIDMetaData appIdMeta;

    std::string key = appIdMetaData.GetKey();
    EXPECT_EQ(key, "appid");
    auto result = MetaDataManager::GetInstance().SaveMeta(key, appIdMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, appIdMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(appIdMetaData.appId, appIdMeta.appId);
    EXPECT_EQ(appIdMetaData.bundleName, appIdMeta.bundleName);
    EXPECT_EQ(appIdMetaData.GetKey(), appIdMeta.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, appIdMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, appIdMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(appIdMetaData.appId, appIdMeta.appId);
    EXPECT_EQ(appIdMetaData.bundleName, appIdMeta.bundleName);
    EXPECT_EQ(appIdMetaData.GetKey(), appIdMeta.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: corruptedMeta
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, corruptedMeta, TestSize.Level1)
{
    CorruptedMetaData corruptedMeta("appid", "ohos.test.demo", "test_store");
    CorruptedMetaData corruptedMetaData;
    corruptedMeta.isCorrupted = true;
    std::string key = corruptedMeta.GetKey();
    EXPECT_EQ(key, "CorruptedMetaData###appid###ohos.test.demo###test_store");

    auto result = MetaDataManager::GetInstance().SaveMeta(key, corruptedMeta, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, corruptedMetaData, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(corruptedMeta.appId, corruptedMetaData.appId);
    EXPECT_EQ(corruptedMeta.bundleName, corruptedMetaData.bundleName);
    EXPECT_EQ(corruptedMeta.storeId, corruptedMetaData.storeId);
    EXPECT_EQ(corruptedMeta.GetKey(), corruptedMetaData.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, corruptedMeta);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, corruptedMetaData);
    EXPECT_TRUE(result);
    EXPECT_EQ(corruptedMeta.appId, corruptedMetaData.appId);
    EXPECT_EQ(corruptedMeta.bundleName, corruptedMetaData.bundleName);
    EXPECT_EQ(corruptedMeta.storeId, corruptedMetaData.storeId);
    EXPECT_EQ(corruptedMeta.GetKey(), corruptedMetaData.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: SecretKeyMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, SecretKeyMetaData001, TestSize.Level1)
{
    SecretKeyMetaData secretKeyMeta;
    SecretKeyMetaData secretKeyMetaData;
    secretKeyMeta.storeType = 1;
    std::initializer_list<std::string> fields = {"time", "skey"};

    std::string key = secretKeyMeta.GetKey(fields);
    EXPECT_EQ(key, "SecretKey###time###skey###SINGLE_KEY");
    std::string backupkey = secretKeyMeta.GetBackupKey(fields);
    EXPECT_EQ(backupkey, "BackupSecretKey###time###skey###");

    auto result = MetaDataManager::GetInstance().SaveMeta(key, secretKeyMeta, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, secretKeyMetaData, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(secretKeyMeta.GetKey(fields), secretKeyMetaData.GetKey(fields));
    EXPECT_EQ(secretKeyMeta.GetBackupKey(fields), secretKeyMetaData.GetBackupKey(fields));

    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, secretKeyMeta);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, secretKeyMetaData);
    EXPECT_TRUE(result);
    EXPECT_EQ(secretKeyMeta.GetKey(fields), secretKeyMetaData.GetKey(fields));
    EXPECT_EQ(secretKeyMeta.GetBackupKey(fields), secretKeyMetaData.GetBackupKey(fields));

    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: SecretKeyMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, SecretKeyMetaData002, TestSize.Level1)
{
    SecretKeyMetaData secretKeyMeta;
    SecretKeyMetaData secretKeyMetaData;
    secretKeyMeta.storeType = 1;
    std::initializer_list<std::string> fields = {"time", "skey"};

    std::string prefix = secretKeyMeta.GetPrefix(fields);
    EXPECT_EQ(prefix, "SecretKey###time###skey###");
    std::string backupprefix = secretKeyMeta.GetBackupPrefix(fields);
    EXPECT_EQ(backupprefix, "BackupSecretKey###time###skey###");

    auto result = MetaDataManager::GetInstance().SaveMeta(prefix, secretKeyMeta, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(prefix, secretKeyMetaData, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(secretKeyMeta.GetPrefix(fields), secretKeyMetaData.GetPrefix(fields));
    EXPECT_EQ(secretKeyMeta.GetBackupPrefix(fields), secretKeyMetaData.GetBackupPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(prefix, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(prefix, secretKeyMeta);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(prefix, secretKeyMetaData);
    EXPECT_TRUE(result);
    EXPECT_EQ(secretKeyMeta.GetPrefix(fields), secretKeyMetaData.GetPrefix(fields));
    EXPECT_EQ(secretKeyMeta.GetBackupPrefix(fields), secretKeyMetaData.GetBackupPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(prefix);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData001, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    StoreMetaData storeMeta;

    std::string key = storeMetaData.GetKey();
    EXPECT_EQ(key, "KvStoreMetaData######100###default######test_store");
    std::string keylocal = storeMetaData.GetKeyLocal();
    EXPECT_EQ(keylocal, "KvStoreMetaDataLocal######100###default######test_store");
    std::initializer_list<std::string> fields = {"100", "appid", "test_store"};
    std::string keyfields = storeMetaData.GetKey(fields);
    EXPECT_EQ(keyfields, "KvStoreMetaData###100###appid###test_store");

    auto result = MetaDataManager::GetInstance().SaveMeta(key, storeMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, storeMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetKey(), storeMeta.GetKey());
    EXPECT_EQ(storeMetaData.GetKeyLocal(), storeMeta.GetKeyLocal());
    EXPECT_EQ(storeMetaData.GetKey(fields), storeMeta.GetKey(fields));

    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, storeMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, storeMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetKey(), storeMeta.GetKey());
    EXPECT_EQ(storeMetaData.GetKeyLocal(), storeMeta.GetKeyLocal());
    EXPECT_EQ(storeMetaData.GetKey(fields), storeMeta.GetKey(fields));

    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData002, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    StoreMetaData storeMeta;

    std::string secretkey = storeMetaData.GetSecretKey();
    EXPECT_EQ(secretkey, "SecretKey###100###default######test_store###0###SINGLE_KEY");
    std::string backupsecretkey = storeMetaData.GetBackupSecretKey();
    EXPECT_EQ(backupsecretkey, "BackupSecretKey###100###default######test_store###0###");

    auto result = MetaDataManager::GetInstance().SaveMeta(secretkey, storeMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(secretkey, storeMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetSecretKey(), storeMeta.GetSecretKey());
    EXPECT_EQ(storeMetaData.GetBackupSecretKey(), storeMeta.GetBackupSecretKey());

    result = MetaDataManager::GetInstance().DelMeta(secretkey, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(secretkey, storeMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(secretkey, storeMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetSecretKey(), storeMeta.GetSecretKey());
    EXPECT_EQ(storeMetaData.GetBackupSecretKey(), storeMeta.GetBackupSecretKey());

    result = MetaDataManager::GetInstance().DelMeta(secretkey);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData003, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    StoreMetaData storeMeta;

    auto storealias = storeMetaData.GetStoreAlias();
    EXPECT_EQ(storealias, "tes***ore");
    std::string strategykey = storeMetaData.GetStrategyKey();
    EXPECT_EQ(strategykey, "StrategyMetaData######100###default######test_store");
    std::initializer_list<std::string> fields = {"100", "appid", "test_store"};
    std::string prefix = storeMetaData.GetPrefix(fields);
    EXPECT_EQ(prefix, "KvStoreMetaData###100###appid###test_store###");

    auto result = MetaDataManager::GetInstance().SaveMeta(strategykey, storeMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(strategykey, storeMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetStrategyKey(), storeMeta.GetStrategyKey());
    EXPECT_EQ(storeMetaData.GetStoreAlias(), storeMeta.GetStoreAlias());
    EXPECT_EQ(storeMetaData.GetPrefix(fields), storeMeta.GetPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(strategykey, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(strategykey, storeMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(strategykey, storeMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetStrategyKey(), storeMeta.GetStrategyKey());
    EXPECT_EQ(storeMetaData.GetStoreAlias(), storeMeta.GetStoreAlias());
    EXPECT_EQ(storeMetaData.GetPrefix(fields), storeMeta.GetPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(strategykey);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData004, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    storeMetaData.version = TEST_CURRENT_VERSION;
    storeMetaData.instanceId = 1;
    StoreMetaData storeMeta;

    std::string key = storeMetaData.GetKey();
    EXPECT_EQ(key, "KvStoreMetaData######100###default######test_store###1");
    std::string keylocal = storeMetaData.GetKeyLocal();
    EXPECT_EQ(keylocal, "KvStoreMetaDataLocal######100###default######test_store###1");

    auto result = MetaDataManager::GetInstance().SaveMeta(key, storeMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, storeMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetKey(), storeMeta.GetKey());
    EXPECT_EQ(storeMetaData.GetKeyLocal(), storeMeta.GetKeyLocal());

    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, storeMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, storeMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetKey(), storeMeta.GetKey());
    EXPECT_EQ(storeMetaData.GetKeyLocal(), storeMeta.GetKeyLocal());

    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData005, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    storeMetaData.version = TEST_CURRENT_VERSION;
    storeMetaData.instanceId = 1;
    StoreMetaData storeMeta;

    std::string secretkey = storeMetaData.GetSecretKey();
    EXPECT_EQ(secretkey, "SecretKey###100###default######test_store###SINGLE_KEY");
    std::string backupsecretkey = storeMetaData.GetBackupSecretKey();
    EXPECT_EQ(backupsecretkey, "BackupSecretKey###100###default######test_store###");
    std::string strategykey = storeMetaData.GetStrategyKey();
    EXPECT_EQ(strategykey, "StrategyMetaData######100###default######test_store###1");

    auto result = MetaDataManager::GetInstance().SaveMeta(secretkey, storeMetaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(secretkey, storeMeta, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetSecretKey(), storeMeta.GetSecretKey());
    EXPECT_EQ(storeMetaData.GetBackupSecretKey(), storeMeta.GetBackupSecretKey());
    EXPECT_EQ(storeMetaData.GetStrategyKey(), storeMeta.GetStrategyKey());

    result = MetaDataManager::GetInstance().DelMeta(secretkey, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(secretkey, storeMetaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(secretkey, storeMeta);
    EXPECT_TRUE(result);
    EXPECT_EQ(storeMetaData.GetSecretKey(), storeMeta.GetSecretKey());
    EXPECT_EQ(storeMetaData.GetBackupSecretKey(), storeMeta.GetBackupSecretKey());
    EXPECT_EQ(storeMetaData.GetStrategyKey(), storeMeta.GetStrategyKey());

    result = MetaDataManager::GetInstance().DelMeta(secretkey);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData006, TestSize.Level1)
{
    StoreMetaData storemetaData1("100", "appid", "test_store");
    StoreMetaData storemetaData2("100", "appid", "test_store");
    StoreMetaData storemetaData3("10", "appid1", "storeid");
    EXPECT_TRUE(storemetaData1 == storemetaData2);
    EXPECT_FALSE(storemetaData1 == storemetaData3);

    storemetaData1.isAutoSync = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isAutoSync = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);

    storemetaData1.isBackup = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isBackup = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);

    storemetaData1.isDirty = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isDirty = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);

    storemetaData1.isEncrypt = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isEncrypt = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);

    storemetaData1.isSearchable = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isSearchable = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);

    storemetaData1.isNeedCompress = true;
    EXPECT_FALSE(storemetaData1 == storemetaData2);
    storemetaData2.isNeedCompress = true;
    EXPECT_TRUE(storemetaData1 == storemetaData2);
}

/**
* @tc.name: StoreMetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StoreMetaData007, TestSize.Level1)
{
    StoreMetaData storemetaData1("100", "appid", "test_store");
    StoreMetaData storemetaData2("100", "appid", "test_store");
    StoreMetaData storemetaData3("10", "appid1", "storeid");
    EXPECT_TRUE(storemetaData1 != storemetaData3);
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isAutoSync = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isAutoSync = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isBackup = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isBackup = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isDirty = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isDirty = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isEncrypt = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isEncrypt = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isSearchable = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isSearchable = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);

    storemetaData1.isNeedCompress = true;
    EXPECT_TRUE(storemetaData1 != storemetaData2);
    storemetaData2.isNeedCompress = true;
    EXPECT_FALSE(storemetaData1 != storemetaData2);
}

/**
* @tc.name: GetStoreInfo
* @tc.desc: test StoreMetaData GetStoreInfo function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, GetStoreInfo, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    storeMetaData.version = TEST_CURRENT_VERSION;
    storeMetaData.instanceId = 1;

    auto result = storeMetaData.GetStoreInfo();
    EXPECT_EQ(result.instanceId, storeMetaData.instanceId);
    EXPECT_EQ(result.bundleName, storeMetaData.bundleName);
    EXPECT_EQ(result.storeName, storeMetaData.storeId);
}

/**
* @tc.name: StrategyMeta001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StrategyMeta001, TestSize.Level1)
{
    auto deviceId = "deviceId";
    StrategyMeta strategyMeta(deviceId, "100", "ohos.test.demo", "test_store");
    std::vector<std::string> local = {"local1"};
    std::vector<std::string> remote = {"remote1"};
    strategyMeta.capabilityRange.localLabel = local;
    strategyMeta.capabilityRange.remoteLabel = remote;
    strategyMeta.capabilityEnabled = true;
    auto result = strategyMeta.IsEffect();
    EXPECT_TRUE(result);
    StrategyMeta strategyMetaData(deviceId, "200", "ohos.test.test", "test_stores");

    std::string key = strategyMeta.GetKey();
    EXPECT_EQ(key, "StrategyMetaData###deviceId###100###default###ohos.test.demo###test_store");
    std::initializer_list<std::string> fields = { deviceId, "100", "default", "ohos.test.demo", "test_store" };
    std::string prefix = strategyMeta.GetPrefix(fields);
    EXPECT_EQ(prefix, "StrategyMetaData###deviceId###100###default###ohos.test.demo###test_store");

    result = MetaDataManager::GetInstance().SaveMeta(key, strategyMeta, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, strategyMetaData, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(strategyMeta.GetKey(), strategyMetaData.GetKey());
    EXPECT_EQ(strategyMeta.GetPrefix(fields), strategyMetaData.GetPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, strategyMeta);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, strategyMetaData);
    EXPECT_TRUE(result);
    EXPECT_EQ(strategyMeta.GetKey(), strategyMetaData.GetKey());
    EXPECT_EQ(strategyMeta.GetPrefix(fields), strategyMetaData.GetPrefix(fields));

    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: StrategyMeta
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, StrategyMeta002, TestSize.Level1)
{
    auto deviceId = "deviceId";
    StrategyMeta strategyMeta(deviceId, "100", "ohos.test.demo", "test_store");
    std::vector<std::string> local = {"local1"};
    std::vector<std::string> remote = {"remote1"};
    strategyMeta.capabilityRange.localLabel = local;
    strategyMeta.capabilityRange.remoteLabel = remote;
    strategyMeta.capabilityEnabled = true;
    auto result = strategyMeta.IsEffect();
    EXPECT_TRUE(result);
    strategyMeta.instanceId = 1;
    StrategyMeta strategyMetaData(deviceId, "200", "ohos.test.test", "test_stores");

    std::string key = strategyMeta.GetKey();
    EXPECT_EQ(key, "StrategyMetaData###deviceId###100###default###ohos.test.demo###test_store###1");

    result = MetaDataManager::GetInstance().SaveMeta(key, strategyMeta, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, strategyMetaData, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(strategyMeta.GetKey(), strategyMetaData.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, strategyMeta);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, strategyMetaData);
    EXPECT_TRUE(result);
    EXPECT_EQ(strategyMeta.GetKey(), strategyMetaData.GetKey());
    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: MetaData
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, MetaData, TestSize.Level1)
{
    StoreMetaData storeMetaData("100", "appid", "test_store");
    SecretKeyMetaData secretKeyMetaData;
    MetaData metaData;
    MetaData metaDataLoad;
    metaData.storeMetaData = storeMetaData;
    metaData.secretKeyMetaData = secretKeyMetaData;
    metaData.storeType = 1;
    std::initializer_list<std::string> fields = {"time", "skey"};
    std::string key = metaData.storeMetaData.GetKey();
    std::string secretkey = metaData.secretKeyMetaData.GetKey(fields);

    auto result = MetaDataManager::GetInstance().SaveMeta(key, metaData, true);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, metaDataLoad, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(key, metaDataLoad.storeMetaData.GetKey());
    EXPECT_EQ(secretkey, metaDataLoad.secretKeyMetaData.GetKey(fields));
    result = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(result);

    result = MetaDataManager::GetInstance().SaveMeta(key, metaData);
    EXPECT_TRUE(result);
    result = MetaDataManager::GetInstance().LoadMeta(key, metaDataLoad);
    EXPECT_TRUE(result);
    EXPECT_EQ(key, metaDataLoad.storeMetaData.GetKey());
    EXPECT_EQ(secretkey, metaDataLoad.secretKeyMetaData.GetKey(fields));
    result = MetaDataManager::GetInstance().DelMeta(key);
    EXPECT_TRUE(result);
}

/**
* @tc.name: CapMetaData
* @tc.desc: test CapMetaData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, CapMetaData, TestSize.Level1)
{
    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::CURRENT_VERSION;
    Serializable::json node1;
    capMetaData.Marshal(node1);
    EXPECT_EQ(node1["version"], CapMetaData::CURRENT_VERSION);

    CapMetaData capMeta;
    capMeta.Unmarshal(node1);
    EXPECT_EQ(capMeta.version, CapMetaData::CURRENT_VERSION);

    CapMetaRow capMetaRow;
    auto key = capMetaRow.GetKeyFor("PEER_DEVICE_ID");
    std::string str = "CapabilityMeta###PEER_DEVICE_ID";
    std::vector<uint8_t> testKey = { str.begin(), str.end() };
    EXPECT_EQ(key, testKey);
}

/**
* @tc.name: UserMetaData
* @tc.desc: test UserMetaData function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, UserMetaData, TestSize.Level1)
{
    UserMetaData userMetaData;
    userMetaData.deviceId = "PEER_DEVICE_ID";

    UserStatus userStatus;
    userStatus.isActive = true;
    userStatus.id = USER_ID1;
    userMetaData.users = { userStatus };
    userStatus.id = USER_ID2;
    userMetaData.users.emplace_back(userStatus);

    Serializable::json node1;
    userMetaData.Marshal(node1);
    EXPECT_EQ(node1["deviceId"], "PEER_DEVICE_ID");

    UserMetaData userMeta;
    userMeta.Unmarshal(node1);
    EXPECT_EQ(userMeta.deviceId, "PEER_DEVICE_ID");

    Serializable::json node2;
    userStatus.Marshal(node2);
    EXPECT_EQ(node2["isActive"], true);
    EXPECT_EQ(node2["id"], USER_ID2);

    UserStatus userUnmarshal;
    userUnmarshal.Unmarshal(node2);
    EXPECT_EQ(userUnmarshal.isActive, true);
    EXPECT_EQ(userUnmarshal.id, USER_ID2);

    UserMetaRow userMetaRow;
    auto key = userMetaRow.GetKeyFor(userMetaData.deviceId);
    EXPECT_EQ(key, "UserMeta###PEER_DEVICE_ID");
}

/**
* @tc.name: CapabilityRange
* @tc.desc: test CapabilityRange function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceMetaDataTest, CapabilityRange, TestSize.Level1)
{
    CapabilityRange capabilityRange;
    std::vector<std::string> local = {"local1"};
    std::vector<std::string> remote = {"remote1"};
    capabilityRange.localLabel = local;
    capabilityRange.remoteLabel = remote;
    Serializable::json node1;
    capabilityRange.Marshal(node1);
    EXPECT_EQ(node1["localLabel"], local);
    EXPECT_EQ(node1["remoteLabel"], remote);

    CapabilityRange capRange;
    capRange.Unmarshal(node1);
    EXPECT_EQ(capRange.localLabel, local);
    EXPECT_EQ(capRange.remoteLabel, remote);
}
} // namespace OHOS::Test
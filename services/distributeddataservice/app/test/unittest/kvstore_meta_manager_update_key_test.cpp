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
#define LOG_TAG "KvstoreMetaManagerUpdateKeyTest"

#include "metadata/meta_data_manager.h"

#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "kvstore_meta_manager.h"
#include "log_print.h"
#include "metadata/device_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/version_meta_data.h"
#include "executor_pool.h"
#include "semaphore_ex.h"
#include "store_types.h"
#include "gtest/gtest.h"
namespace {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using KvStoreMetaManager = OHOS::DistributedKv::KvStoreMetaManager;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
class KvstoreMetaManagerUpdateKeyTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        auto executors = std::make_shared<OHOS::ExecutorPool>(1, 0);
        Bootstrap::GetInstance().LoadComponents();
        Bootstrap::GetInstance().LoadDirectory();
        DeviceManagerAdapter::GetInstance().Init(executors);
        KvStoreMetaManager::GetInstance().BindExecutor(executors);
        KvStoreMetaManager::GetInstance().InitMetaParameter();
    }
    static void TearDownTestCase()
    {
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

/**
* @tc.name: keyUpdataTestWithUuid
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: cjx
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest001, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    DeviceMetaData deviceMetaData;
    deviceMetaData.newUuid = "KeyUpdataTest001_oldUuid";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(deviceMetaData.GetKey(), deviceMetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(deviceMetaData.GetKey(), deviceMetaData, true));

    StoreMetaData metaData;
    metaData.deviceId = "KeyUpdataTest001_oldUuid";
    metaData.user = "KeyUpdataTest001_user";
    metaData.bundleName = "KeyUpdataTest001_bundleName";
    metaData.storeId = "KeyUpdataTest001_storeId";
    metaData.dataDir = "KeyUpdataTest001_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));
    StoreMetaDataLocal metaDataLocal;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    SecretKeyMetaData secretKeymetaData;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));

    VersionMetaData versionMeta;
    versionMeta.version = VersionMetaData::UPDATE_SYNC_META_VERSION;
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);

    KvStoreMetaManager::GetInstance().InitMetaData();
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), metaDataLocal, true));

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKey(), secretKeymetaData, true));

    StoreMetaMapping storeMetaMapping(metaData);
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: cjx
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest002, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest002_user";
    metaData.bundleName = "KeyUpdataTest002_bundleName";
    metaData.storeId = "KeyUpdataTest002_storeId";
    metaData.dataDir = "KeyUpdataTest002_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));
    StoreMetaDataLocal metaDataLocal;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    SecretKeyMetaData secretKeymetaData;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));

    VersionMetaData versionMeta;
    versionMeta.version = VersionMetaData::UPDATE_SYNC_META_VERSION;
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), metaDataLocal, true));

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKey(), secretKeymetaData, true));

    StoreMetaMapping storeMetaMapping(metaData);
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: cjx
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest003, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest003_user";
    metaData.bundleName = "KeyUpdataTest003_bundleName";
    metaData.storeId = "KeyUpdataTest003_storeId";
    metaData.dataDir = "KeyUpdataTest003_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: my
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest004, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest004_user";
    metaData.bundleName = "KeyUpdataTest004_bundleName";
    metaData.storeId = "KeyUpdataTest004_storeId";
    metaData.dataDir = "KeyUpdataTest004_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    VersionMetaData versionMeta;
    MetaDataManager::GetInstance().DelMeta(versionMeta.GetKey(), true);

    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: my
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest005, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest005_user";
    metaData.bundleName = "KeyUpdataTest005_bundleName";
    metaData.storeId = "KeyUpdataTest005_storeId";
    metaData.dataDir = "KeyUpdataTest005_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    VersionMetaData versionMeta;
    versionMeta.version = 3; // Set version to 3.
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: my
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest006, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();
    StoreMetaData metaData;

    VersionMetaData versionMeta;
    versionMeta.version = VersionMetaData::UPDATE_SYNC_META_VERSION;
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
}

/**
* @tc.name: KeyUpdataTest
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: my
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest007, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();

    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest007_user";
    metaData.bundleName = "KeyUpdataTest007_bundleName";
    metaData.storeId = "KeyUpdataTest007_storeId";
    metaData.dataDir = "KeyUpdataTest007_dataDir";
    StoreMetaDataLocal metaDataLocal;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));

    VersionMetaData versionMeta;
    versionMeta.version = VersionMetaData::UPDATE_SYNC_META_VERSION;
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    KvStoreMetaManager::GetInstance().InitMetaData();

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), metaDataLocal, true));

    StoreMetaMapping storeMetaMapping(metaData);
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true));
}

/**
* @tc.name: keyUpdataTestWithUuid
* @tc.desc: key updata test
* @tc.type: FUNC
* @tc.require:
* @tc.author: cjx
*/
HWTEST_F(KvstoreMetaManagerUpdateKeyTest, KeyUpdataTest008, TestSize.Level1)
{
    auto store = KvStoreMetaManager::GetInstance().GetMetaKvStore();

    StoreMetaData metaData;
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = "KeyUpdataTest008_user";
    metaData.bundleName = "KeyUpdataTest008_bundleName";
    metaData.storeId = "KeyUpdataTest008_storeId";
    metaData.dataDir = "KeyUpdataTest008_dataDir";
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyWithoutPath(), metaData));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));
    StoreMetaDataLocal metaDataLocal;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    SecretKeyMetaData secretKeymetaData;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));

    VersionMetaData versionMeta;
    versionMeta.version = VersionMetaData::CURRENT_VERSION;
    MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);

    KvStoreMetaManager::GetInstance().InitMetaData();
    metaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData));

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocalWithoutPath(), metaDataLocal, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), metaDataLocal, true));

    EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKeyWithoutPath(), secretKeymetaData, true));
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(metaData.GetSecretKey(), secretKeymetaData, true));

    StoreMetaMapping storeMetaMapping(metaData);
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(storeMetaMapping.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyWithoutPath()));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true));
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true));
}
} // namespace
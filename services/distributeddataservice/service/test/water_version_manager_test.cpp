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
#include "water_version_manager.h"

#include "accesstoken_kit.h"
#include "block_data.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "gtest/gtest.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "mock/checker_mock.h"
#include "mock/db_store_mock.h"
#include "nativetoken_kit.h"
#include "serializable/serializable.h"
#include "token_setproc.h"
#include "utils/constant.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS::Security::AccessToken;
using WvManager = WaterVersionManager;
using DMAdapter = DeviceManagerAdapter;
using WaterVersionMetaData = WvManager::WaterVersionMetaData;

namespace OHOS::Test {
namespace DistributedDataTest {
static std::vector<std::pair<std::string, std::string>> staticStores_ = { { "bundle0", "store0" },
    { "bundle1", "store0" }, { "bundle2", "store0" } };
static std::vector<std::pair<std::string, std::string>> dynamicStores_ = { { "bundle0", "store1" },
    { "bundle3", "store0" }, { "bundle4", "store0" } };
class WaterVersionManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *TEST_DEVICE = "14a0a92a428005db27c40bad46bf145fede38ec37effe0347cd990fcb031f320";
    static void GrantPermissionNative();
    void InitMetaData();

    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static WaterVersionMetaData dynamicMeta_;
    static WaterVersionMetaData staticMeta_;
    static CheckerMock instance_;
};

void WaterVersionManagerTest::GrantPermissionNative()
{
    const char **perms = new const char *[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "WaterVersionManagerTest",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}
CheckerMock WaterVersionManagerTest::instance_;
std::shared_ptr<DBStoreMock> WaterVersionManagerTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
WaterVersionMetaData WaterVersionManagerTest::staticMeta_;
WaterVersionMetaData WaterVersionManagerTest::dynamicMeta_;
void WaterVersionManagerTest::SetUpTestCase(void)
{
    size_t max = 12;
    size_t min = 5;
    DeviceManagerAdapter::GetInstance().Init(std::make_shared<OHOS::ExecutorPool>(max, min));
    GrantPermissionNative();
    Bootstrap::GetInstance().LoadCheckers();
    std::vector<CheckerManager::StoreInfo> dynamicStores;
    for (auto &[bundle, store] : dynamicStores_) {
        dynamicStores.push_back({ 0, 0, bundle, store });
        instance_.SetDynamic(dynamicStores);
    }
    std::vector<CheckerManager::StoreInfo> staticStores;
    for (auto &[bundle, store] : staticStores_) {
        staticStores.push_back({ 0, 0, bundle, store });
        instance_.SetStatic(staticStores);
    }
    WaterVersionManager::GetInstance().Init();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);

    staticMeta_.deviceId = TEST_DEVICE;
    staticMeta_.version = WvManager::WaterVersionMetaData::DEFAULT_VERSION;
    staticMeta_.waterVersion = 0;
    staticMeta_.type = WvManager::STATIC;
    staticMeta_.keys.clear();
    for (auto &it : staticStores_) {
        staticMeta_.keys.push_back(Constant::Join(it.first, Constant::KEY_SEPARATOR, { it.second }));
    }
    staticMeta_.infos = { staticMeta_.keys.size(), std::vector<uint64_t>(staticMeta_.keys.size(), 0) };
    dynamicMeta_ = staticMeta_;
    dynamicMeta_.keys.clear();
    for (auto &it : dynamicStores_) {
        dynamicMeta_.keys.push_back(Constant::Join(it.first, Constant::KEY_SEPARATOR, { it.second }));
    }
    dynamicMeta_.infos = { dynamicMeta_.keys.size(), std::vector<uint64_t>(dynamicMeta_.keys.size(), 0) };
    dynamicMeta_.type = WvManager::DYNAMIC;
}

void WaterVersionManagerTest::TearDownTestCase(void) {}

void WaterVersionManagerTest::SetUp()
{
    InitMetaData();
}

void WaterVersionManagerTest::TearDown()
{
    WvManager::GetInstance().DelWaterVersion(TEST_DEVICE);
    WvManager::GetInstance().DelWaterVersion(DMAdapter::GetInstance().GetLocalDevice().uuid);
    MetaDataManager::GetInstance().DelMeta(staticMeta_.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(dynamicMeta_.GetKey(), true);
    MatrixMetaData matrixMetaData;
    matrixMetaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    MetaDataManager::GetInstance().DelMeta(matrixMetaData.GetKey(), true);
}

void WaterVersionManagerTest::InitMetaData()
{
    MetaDataManager::GetInstance().SaveMeta(staticMeta_.GetKey(), staticMeta_, true);
    MetaDataManager::GetInstance().SaveMeta(dynamicMeta_.GetKey(), dynamicMeta_, true);
}

/**
* @tc.name: SetWaterVersionTest1
* @tc.desc: updating remote device water version once;
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, SetWaterVersionTest1, TestSize.Level0)
{
    std::string waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, WvManager::STATIC);
    ASSERT_EQ(waterVersion, Serializable::Marshall(staticMeta_)) << "meta: " << waterVersion;
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;

    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(staticStores_[0].first, staticStores_[0].second,
        Serializable::Marshall(meta)))
        << Serializable::Marshall(meta);

    waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, WvManager::STATIC);
    ASSERT_EQ(Serializable::Marshall(meta), waterVersion);
    auto [success, version] = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_TRUE(success && version == 1)
        << "success:" << success << " version:" << version << " meta: " << meta.ToAnonymousString();
}

/**
* @tc.name: SetWaterVersionTest2
* @tc.desc: Continuously updating remote device water version more than once;
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, SetWaterVersionTest2, TestSize.Level0)
{
    std::string waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, WvManager::STATIC);
    ASSERT_EQ(waterVersion, Serializable::Marshall(staticMeta_)) << "meta: " << waterVersion;
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    // first store update
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(staticStores_[0].first, staticStores_[0].second,
        Serializable::Marshall(meta)));
    auto [_, version] = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_EQ(version, 1);

    // second store update
    meta.infos[1] = { 1, 2, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(staticStores_[1].first, staticStores_[1].second,
        Serializable::Marshall(meta)));
    std::tie(_, version) = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_EQ(version, 2);
}

/**
* @tc.name: SetWaterVersionTest3
* @tc.desc: A updates first, but B synchronizes first
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, SetWaterVersionTest3, TestSize.Level0)
{
    std::string waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, WvManager::DYNAMIC);
    ASSERT_EQ(waterVersion, Serializable::Marshall(dynamicMeta_)) << "meta: " << waterVersion;
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //bundle2 updated later, but sync first. Do not update waterVersion
    meta.infos[1] = { 1, 2, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[1].first, dynamicStores_[1].second,
        Serializable::Marshall(meta)));
    auto res = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_TRUE(res.first && res.second == 0)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();

    //bundle1 updated earlier, but sync later. update waterVersion
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[0].first, dynamicStores_[0].second,
        Serializable::Marshall(meta)));
    res = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::DYNAMIC);
    EXPECT_TRUE(res.first && res.second == 2)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();
}

/**
* @tc.name: SetWaterVersionTest4
* @tc.desc: updates twice, but sync once
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, SetWaterVersionTest4, TestSize.Level0)
{
    std::string waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, WvManager::STATIC);
    ASSERT_EQ(waterVersion, Serializable::Marshall(staticMeta_)) << "meta: " << waterVersion;
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //bundle1 updated twice, but sync once. update waterVersion
    meta.infos[0] = { 2, 0, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(staticStores_[0].first, staticStores_[0].second,
        Serializable::Marshall(meta)));

    auto res = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_TRUE(res.first && res.second == 2)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();
}

/**
* @tc.name: SetWaterVersionTest5
* @tc.desc: Update(upload) multiple times, sync(download) fewer times
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, SetWaterVersionTest5, TestSize.Level0)
{
    auto type = WvManager::DYNAMIC;
    std::string waterVersion = WvManager::GetInstance().GetWaterVersion(TEST_DEVICE, type);
    ASSERT_EQ(waterVersion, Serializable::Marshall(dynamicMeta_)) << "meta: " << waterVersion;
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //order: 0(u)->1(u)->0(u)->2(u)->1(s)->2(s)->1(u)->1(u)->0(s)
    //1 sync
    meta.infos[0] = { 1, 0, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 0, 0, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[1].first, dynamicStores_[1].second,
        Serializable::Marshall(meta)));

    auto res = WvManager::GetInstance().GetVersion(TEST_DEVICE, type);
    EXPECT_TRUE(res.first && res.second == 0)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();

    //2 sync
    meta.infos[0] = { 3, 2, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 3, 2, 4 };
    meta.waterVersion = 4;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[2].first, dynamicStores_[2].second,
        Serializable::Marshall(meta)));
    res = WvManager::GetInstance().GetVersion(TEST_DEVICE, type);
    EXPECT_TRUE(res.first && res.second == 0)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();

    //0 sync
    meta.infos[0] = { 3, 2, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 0, 0, 0 };
    meta.waterVersion = 3;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[0].first, dynamicStores_[0].second,
        Serializable::Marshall(meta)));
    res = WvManager::GetInstance().GetVersion(TEST_DEVICE, type);
    EXPECT_TRUE(res.first && res.second == 4)
        << "success:" << res.first << " version:" << res.second << " meta: " << meta.ToAnonymousString();
}

/**
* @tc.name: GenerateWaterVersionTest1
* @tc.desc: GenerateWaterVersion once, add one to the waterVersion
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, GenerateWaterVersionTest1, TestSize.Level0)
{
    std::string waterVersion =
        WvManager::GetInstance().GenerateWaterVersion(staticStores_[0].first, staticStores_[0].second);
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 1) << "waterVersion: " << waterVersion;

    waterVersion = WvManager::GetInstance().GenerateWaterVersion(staticStores_[1].first, staticStores_[1].second);
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 2) << "waterVersion: " << waterVersion;

    waterVersion = WvManager::GetInstance().GenerateWaterVersion(staticStores_[0].first, staticStores_[0].second);
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 3) << "waterVersion: " << waterVersion;

    MatrixMetaData matrixMetaData;
    matrixMetaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto key = matrixMetaData.GetKey();
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(key, matrixMetaData, true));
    EXPECT_EQ(matrixMetaData.statics & 0xFFF0, 3 << 4);
}

/**
* @tc.name: GenerateWaterVersionTest2
* @tc.desc: Multiple threads simultaneously GenerateWaterVersion
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, GenerateWaterVersionTest2, TestSize.Level0)
{
    auto executorPool = std::make_shared<ExecutorPool>(5, 5);
    auto len = dynamicStores_.size();
    for (size_t i = 0; i < 10; ++i) {
        executorPool->Execute([index = i % len] {
            WvManager::GetInstance().GenerateWaterVersion(dynamicStores_[index].first, dynamicStores_[index].second);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto [success, version] =
        WvManager::GetInstance().GetVersion(DMAdapter::GetInstance().GetLocalDevice().uuid, WvManager::DYNAMIC);
    EXPECT_EQ(version, 10);

    MatrixMetaData matrixMetaData;
    matrixMetaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto key = matrixMetaData.GetKey();
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(key, matrixMetaData, true));
    EXPECT_EQ(matrixMetaData.dynamic & 0xFFF0, version << 4);
}

/**
* @tc.name: GetWaterVersionTest1
* @tc.desc: GetWaterVersion by different key
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, GetWaterVersionTest1, TestSize.Level0)
{
    auto len = staticStores_.size();
    for (size_t i = 0; i < 10; ++i) {
        auto bundle = staticStores_[i % len].first;
        auto store = staticStores_[i % len].second;
        ASSERT_FALSE(WvManager::GetInstance().GenerateWaterVersion(bundle, store).empty());
    }
    for (size_t i = 0; i < len; i++) {
        auto waterVersion = WvManager::GetInstance().GetWaterVersion(staticStores_[i].first, staticStores_[i].second);
        WvManager::WaterVersionMetaData meta;
        ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
        EXPECT_EQ(meta.waterVersion, 10 / len * len + i + 1 - len * (10 % len <= i ? 1 : 0));
    }
}

/**
* @tc.name: MixCallTest1
* @tc.desc: use GenerateWaterVersion to SetWaterVersion
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, MixCallTest1, TestSize.Level0)
{
    std::string waterVersion =
        WvManager::GetInstance().GenerateWaterVersion(staticStores_[0].first, staticStores_[0].second);
    WvManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 1) << "waterVersion: " << waterVersion;

    meta.deviceId = TEST_DEVICE;
    EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(staticStores_[0].first, staticStores_[0].second,
        Serializable::Marshall(meta)));

    auto [_, version] = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::STATIC);
    EXPECT_EQ(version, 1) << "version: " << version;
}

/**
* @tc.name: MixCallTest2
* @tc.desc: use GenerateWaterVersion to SetWaterVersion and GenerateWaterVersion more than once
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(WaterVersionManagerTest, MixCallTest2, TestSize.Level0)
{
    bool success = false;
    uint64_t version = 0;
    for (int i = 1; i < 11; ++i) {
        int index = i % dynamicMeta_.keys.size();
        std::string waterVersion =
            WvManager::GetInstance().GenerateWaterVersion(dynamicStores_[index].first, dynamicStores_[index].second);
        WvManager::WaterVersionMetaData meta;
        ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
        EXPECT_EQ(meta.waterVersion, i) << "waterVersion: " << waterVersion;
        meta.deviceId = TEST_DEVICE;
        EXPECT_TRUE(WvManager::GetInstance().SetWaterVersion(dynamicStores_[index].first, dynamicStores_[index].second,
            Serializable::Marshall(meta)));

        std::tie(success, version) = WvManager::GetInstance().GetVersion(TEST_DEVICE, WvManager::DYNAMIC);
        EXPECT_TRUE(success && version == i)
            << "i:" << i << " success:" << success << " version:" << version << " meta: " << meta.ToAnonymousString();
    }
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
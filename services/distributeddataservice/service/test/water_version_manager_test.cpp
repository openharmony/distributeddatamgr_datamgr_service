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

#include "checker/checker_manager.h"
#include "block_data.h"
#include "device_manager_adapter.h"
#include "bootstrap.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "metadata/meta_data_manager.h"
#include "mock/db_store_mock.h"
#include "serializable/serializable.h"
#include "utils/constant.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using DMAdapter = DeviceManagerAdapter;
using namespace DistributedDB;
using WaterVersionMetaData = WaterVersionManager::WaterVersionMetaData;

namespace OHOS::Test {
namespace DistributedDataTest {
class TestChecker;
class WaterVersionManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *TEST_DEVICE = "14a0a92a428005db27c40bad46bf145fede38ec37effe0347cd990fcb031f320";
    void InitMetaData();

    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static WaterVersionMetaData meta_;
    static std::vector<std::pair<std::string, std::string>> stores_;
};
class TestChecker : public CheckerManager::Checker {
public:
    TestChecker() noexcept
    {
        CheckerManager::GetInstance().RegisterPlugin(
            "SystemChecker", [this]() -> auto { return this; });
    }
    ~TestChecker() {}
    void Initialize() override {}
    bool SetTrustInfo(const CheckerManager::Trust &trust) override
    {
        return true;
    }
    std::string GetAppId(const CheckerManager::StoreInfo &info) override
    {
        return info.bundleName;
    }
    bool IsValid(const CheckerManager::StoreInfo &info) override
    {
        return true;
    }
    bool SetDistrustInfo(const CheckerManager::Distrust &distrust) override
    {
        return true;
    };

    bool IsDistrust(const CheckerManager::StoreInfo &info) override
    {
        return true;
    }
    bool SetDynamicStores(const vector<CheckerManager::StoreInfo> &storeInfos) override
    {
        return false;
    }
    bool SetStaticStores(const vector<CheckerManager::StoreInfo> &storeInfos) override
    {
        return false;
    }
    vector<CheckerManager::StoreInfo> GetDynamicStores() override
    {
        return {};
    }
    vector<CheckerManager::StoreInfo> GetStaticStores() override
    {
        return { { 0, 0, "bundle0", "store0" }, { 0, 0, "bundle1", "store0" }, { 0, 0, "bundle2", "store0" } };
    }
    bool IsDynamicStores(const CheckerManager::StoreInfo &info) override
    {
        return false;
    }
    bool IsStaticStores(const CheckerManager::StoreInfo &info) override
    {
        return false;
    }
};
TestChecker instance_;
std::shared_ptr<DBStoreMock> WaterVersionManagerTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
WaterVersionMetaData WaterVersionManagerTest::meta_;
std::vector<std::pair<std::string, std::string>> WaterVersionManagerTest::stores_ = { { "bundle0", "store0" },
    { "bundle1", "store0" }, { "bundle2", "store0" } };
void WaterVersionManagerTest::SetUpTestCase(void)
{
    Bootstrap::GetInstance().LoadCheckers();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);

    meta_.deviceId = TEST_DEVICE;
    meta_.version = WaterVersionManager::WaterVersionMetaData::DEFAULT_VERSION;
    meta_.waterVersion = 0;
    meta_.type = WaterVersionManager::STATIC;
    meta_.keys.clear();
    for (auto &it : stores_) {
        meta_.keys.push_back(Constant::Join(it.first, Constant::KEY_SEPARATOR, { it.second }));
    }
    meta_.infos = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
}

void WaterVersionManagerTest::TearDownTestCase(void) {}

void WaterVersionManagerTest::SetUp()
{
    InitMetaData();
}

void WaterVersionManagerTest::TearDown()
{
    WaterVersionManager::GetInstance().DelWaterVersion(TEST_DEVICE);
    WaterVersionManager::GetInstance().DelWaterVersion(DMAdapter::GetInstance().GetLocalDevice().uuid);
    MetaDataManager::GetInstance().DelMeta(meta_.GetKey(), true);
}

void WaterVersionManagerTest::InitMetaData()
{
    MetaDataManager::GetInstance().SaveMeta(meta_.GetKey(), meta_, true);
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
    std::string waterVersion;
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(waterVersion, Serializable::Marshall(meta_));
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;

    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)))
        << Serializable::Marshall(meta);
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(Serializable::Marshall(meta), waterVersion);

    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 1) << "meta: " << meta.ToAnonymousString();
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
    std::string waterVersion;
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(waterVersion, Serializable::Marshall(meta_));
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    // first store update
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)));
    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 1);

    // second store update
    meta.infos[1] = { 1, 2, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[1].first, stores_[1].second,
        Serializable::Marshall(meta)));
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
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
    std::string waterVersion;
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(waterVersion, Serializable::Marshall(meta_));
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //bundle2 updated later, but sync first. Do not update waterVersion
    meta.infos[1] = { 1, 2, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[1].first, stores_[1].second,
        Serializable::Marshall(meta)));
    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 0);

    //bundle1 updated earlier, but sync later. update waterVersion
    meta.infos[0] = { 1, 0, 0 };
    meta.waterVersion = 1;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)));
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 2);
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
    std::string waterVersion;
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(waterVersion, Serializable::Marshall(meta_));
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //bundle1 updated twice, but sync once. update waterVersion
    meta.infos[0] = { 2, 0, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)));
    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 2);
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
    std::string waterVersion;
    ASSERT_TRUE(
        WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, waterVersion));
    ASSERT_EQ(waterVersion, Serializable::Marshall(meta_));
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta));
    EXPECT_EQ(meta.GetVersion(), 0) << "meta: " << Serializable::Marshall(meta);

    //order: 0(u)->1(u)->0(u)->2(u)->1(s)->2(s)->1(u)->1(u)->0(s)
    //1 sync
    meta.infos[0] = { 1, 0, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 0, 0, 0 };
    meta.waterVersion = 2;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[1].first, stores_[1].second,
        Serializable::Marshall(meta)));
    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 0);

    //2 sync
    meta.infos[0] = { 3, 2, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 3, 2, 4 };
    meta.waterVersion = 4;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[2].first, stores_[2].second,
        Serializable::Marshall(meta)));
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 0);

    //0 sync
    meta.infos[0] = { 3, 2, 0 };
    meta.infos[1] = { 1, 2, 0 };
    meta.infos[2] = { 0, 0, 0 };
    meta.waterVersion = 3;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)));
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 4);
    std::cout<<Serializable::Marshall(meta)<<std::endl;
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
        WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[0].first, stores_[0].second);
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 1) << "waterVersion: " << waterVersion;

    waterVersion = WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[1].first, stores_[1].second);
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 2) << "waterVersion: " << waterVersion;

    waterVersion = WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[0].first, stores_[0].second);
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 3) << "waterVersion: " << waterVersion;
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
    for (int i = 0; i < 10; ++i) {
        executorPool->Execute([] {
            WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[0].first, stores_[0].second);
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(DMAdapter::GetInstance().GetLocalDevice().uuid,
        WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 10);
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
        WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[0].first, stores_[0].second);
    WaterVersionManager::WaterVersionMetaData meta;
    ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
    EXPECT_EQ(meta.waterVersion, 1) << "waterVersion: " << waterVersion;

    meta.deviceId = TEST_DEVICE;
    EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[0].first, stores_[0].second,
        Serializable::Marshall(meta)));

    uint64_t version;
    EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
    EXPECT_EQ(version, 1) << "waterVersion: " << waterVersion;
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
    for (int i = 1; i < 11; ++i) {
        std::string waterVersion =
            WaterVersionManager::GetInstance().GenerateWaterVersion(stores_[i % meta_.keys.size()].first,
                stores_[i % meta_.keys.size()].second);
        WaterVersionManager::WaterVersionMetaData meta;
        ASSERT_TRUE(Serializable::Unmarshall(waterVersion, meta)) << "waterVersion: " << waterVersion;
        EXPECT_EQ(meta.waterVersion, i) << "waterVersion: " << waterVersion;
        meta.deviceId = TEST_DEVICE;
        EXPECT_TRUE(WaterVersionManager::GetInstance().SetWaterVersion(stores_[i % meta_.keys.size()].first,
            stores_[i % meta_.keys.size()].second, Serializable::Marshall(meta)));

        uint64_t version;
        EXPECT_TRUE(WaterVersionManager::GetInstance().GetWaterVersion(TEST_DEVICE, WaterVersionManager::STATIC, version));
        EXPECT_EQ(version, i) << "waterVersion: " << waterVersion;
    }
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
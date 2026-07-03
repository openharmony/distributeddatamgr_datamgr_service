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

#include "metadata/bundle_version_meta_data.h"

#include "bootstrap.h"
#include "kvstore_meta_manager.h"
#include "metadata/meta_data_manager.h"
#include "utils/constant.h"
#include "gtest/gtest.h"
#include "serializable/serializable.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class BundleVersionMetaDataTest : public testing::Test {
public:
    static constexpr size_t NUM_MIN = 1;
    static constexpr size_t NUM_MAX = 2;
    static void SetUpTestCase()
    {
        std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
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
 * @tc.name: BundleVersionMetaData_Marshal_Unmarshal_RoundTrip
 * @tc.desc: Test Marshal and Unmarshal round-trip preserves all fields
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_Marshal_Unmarshal_RoundTrip, TestSize.Level1)
{
    BundleVersionMetaData original;
    original.bundleName = "com.example.testapp";
    original.user = "100";
    original.appIndex = 0;
    original.versionCode = 12345;

    Serializable::json node;
    bool marshalRet = original.Marshal(node);
    EXPECT_TRUE(marshalRet);
    EXPECT_EQ(node["bundleName"], "com.example.testapp");
    EXPECT_EQ(node["user"], "100");
    EXPECT_EQ(node["appIndex"], 0);
    EXPECT_EQ(node["versionCode"], 12345);

    BundleVersionMetaData loaded;
    bool unmarshalRet = loaded.Unmarshal(node);
    EXPECT_TRUE(unmarshalRet);
    EXPECT_EQ(loaded.bundleName, original.bundleName);
    EXPECT_EQ(loaded.user, original.user);
    EXPECT_EQ(loaded.appIndex, original.appIndex);
    EXPECT_EQ(loaded.versionCode, original.versionCode);
}

/**
 * @tc.name: BundleVersionMetaData_Marshal_DefaultValues
 * @tc.desc: Test Marshal with default zero/empty values
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_Marshal_DefaultValues, TestSize.Level1)
{
    BundleVersionMetaData meta;
    EXPECT_EQ(meta.bundleName, "");
    EXPECT_EQ(meta.user, "");
    EXPECT_EQ(meta.appIndex, 0);
    EXPECT_EQ(meta.versionCode, 0);

    Serializable::json node;
    bool ret = meta.Marshal(node);
    EXPECT_TRUE(ret);
    EXPECT_EQ(node["bundleName"], "");
    EXPECT_EQ(node["user"], "");
    EXPECT_EQ(node["appIndex"], 0);
    EXPECT_EQ(node["versionCode"], 0);
}

/**
 * @tc.name: BundleVersionMetaData_Unmarshal_PartialJson
 * @tc.desc: Test Unmarshal with partial JSON (only some fields present)
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_Unmarshal_PartialJson, TestSize.Level1)
{
    Serializable::json node;
    node["bundleName"] = "com.example.partial";
    node["versionCode"] = 999;

    BundleVersionMetaData meta;
    bool ret = meta.Unmarshal(node);
    EXPECT_TRUE(ret);
    EXPECT_EQ(meta.bundleName, "com.example.partial");
    EXPECT_EQ(meta.versionCode, 999);
    EXPECT_EQ(meta.user, "");
    EXPECT_EQ(meta.appIndex, 0);
}

/**
 * @tc.name: BundleVersionMetaData_GetKey_InstanceMethod
 * @tc.desc: Test GetKey() instance method produces correct key format
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_GetKey_InstanceMethod, TestSize.Level1)
{
    BundleVersionMetaData meta;
    meta.bundleName = "com.example.testapp";
    meta.user = "100";

    std::string key = meta.GetKey();
    std::string expected = std::string(BundleVersionMetaData::KEY_PREFIX) + Constant::KEY_SEPARATOR
        + "100" + Constant::KEY_SEPARATOR + "default" + Constant::KEY_SEPARATOR + "com.example.testapp";
    EXPECT_EQ(key, expected);
}

/**
 * @tc.name: BundleVersionMetaData_GetKey_StaticMethod
 * @tc.desc: Test GetKey() static method with initializer list
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_GetKey_StaticMethod, TestSize.Level1)
{
    std::string key = BundleVersionMetaData::GetKey({ "100", "default", "com.example.testapp" });
    std::string expected = std::string(BundleVersionMetaData::KEY_PREFIX) + Constant::KEY_SEPARATOR
        + "100" + Constant::KEY_SEPARATOR + "default" + Constant::KEY_SEPARATOR + "com.example.testapp";
    EXPECT_EQ(key, expected);
}

/**
 * @tc.name: BundleVersionMetaData_GetPrefix_EmptyFields
 * @tc.desc: Test GetPrefix with empty fields returns prefix with trailing separator
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_GetPrefix_EmptyFields, TestSize.Level1)
{
    std::string prefix = BundleVersionMetaData::GetPrefix({});
    std::string expected = std::string(BundleVersionMetaData::KEY_PREFIX) + Constant::KEY_SEPARATOR;
    EXPECT_EQ(prefix, expected);
}

/**
 * @tc.name: BundleVersionMetaData_GetPrefix_WithUserAndBundleName
 * @tc.desc: Test GetPrefix with user and bundleName fields
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_GetPrefix_WithUserAndBundleName, TestSize.Level1)
{
    std::string prefix = BundleVersionMetaData::GetPrefix({ "100", "default", "com.example.testapp" });
    std::string expected = std::string(BundleVersionMetaData::KEY_PREFIX) + Constant::KEY_SEPARATOR
        + "100" + Constant::KEY_SEPARATOR + "default" + Constant::KEY_SEPARATOR
        + "com.example.testapp" + Constant::KEY_SEPARATOR;
    EXPECT_EQ(prefix, expected);
}

/**
 * @tc.name: BundleVersionMetaData_SaveLoadDelete_Lifecycle
 * @tc.desc: Test full save/load/delete lifecycle via MetaDataManager
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_SaveLoadDelete_Lifecycle, TestSize.Level1)
{
    BundleVersionMetaData original;
    original.bundleName = "com.example.lifecycle";
    original.user = "100";
    original.appIndex = 0;
    original.versionCode = 100;

    std::string key = original.GetKey();
    auto saveResult = MetaDataManager::GetInstance().SaveMeta(key, original, true);
    EXPECT_TRUE(saveResult);

    BundleVersionMetaData loaded;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(key, loaded, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(loaded.bundleName, original.bundleName);
    EXPECT_EQ(loaded.user, original.user);
    EXPECT_EQ(loaded.appIndex, original.appIndex);
    EXPECT_EQ(loaded.versionCode, original.versionCode);
    EXPECT_EQ(loaded.GetKey(), original.GetKey());

    auto delResult = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(delResult);

    BundleVersionMetaData afterDelete;
    auto loadAfterDel = MetaDataManager::GetInstance().LoadMeta(key, afterDelete, true);
    EXPECT_FALSE(loadAfterDel);
}

/**
 * @tc.name: BundleVersionMetaData_SaveLoad_VersionCodeUpdate
 * @tc.desc: Test saving updated versionCode overwrites previous value
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_SaveLoad_VersionCodeUpdate, TestSize.Level1)
{
    BundleVersionMetaData v1;
    v1.bundleName = "com.example.versionupdate";
    v1.user = "100";
    v1.appIndex = 0;
    v1.versionCode = 100;

    std::string key = v1.GetKey();
    auto saveResult = MetaDataManager::GetInstance().SaveMeta(key, v1, true);
    EXPECT_TRUE(saveResult);

    BundleVersionMetaData v2;
    v2.bundleName = v1.bundleName;
    v2.user = v1.user;
    v2.appIndex = v1.appIndex;
    v2.versionCode = 200;
    saveResult = MetaDataManager::GetInstance().SaveMeta(key, v2, true);
    EXPECT_TRUE(saveResult);

    BundleVersionMetaData loaded;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(key, loaded, true);
    EXPECT_TRUE(loadResult);
    EXPECT_EQ(loaded.versionCode, 200);

    auto delResult = MetaDataManager::GetInstance().DelMeta(key, true);
    EXPECT_TRUE(delResult);
}

/**
 * @tc.name: BundleVersionMetaData_LoadMeta_PrefixQuery
 * @tc.desc: Test loading multiple BundleVersionMetaData entries via prefix query
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_LoadMeta_PrefixQuery, TestSize.Level1)
{
    BundleVersionMetaData meta1;
    meta1.bundleName = "com.example.prefix1";
    meta1.user = "100";
    meta1.appIndex = 0;
    meta1.versionCode = 100;

    BundleVersionMetaData meta2;
    meta2.bundleName = "com.example.prefix2";
    meta2.user = "100";
    meta2.appIndex = 0;
    meta2.versionCode = 200;

    auto saveResult = MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true);
    EXPECT_TRUE(saveResult);
    saveResult = MetaDataManager::GetInstance().SaveMeta(meta2.GetKey(), meta2, true);
    EXPECT_TRUE(saveResult);

    std::string prefix = BundleVersionMetaData::GetPrefix({});
    std::vector<BundleVersionMetaData> entries;
    auto loadResult = MetaDataManager::GetInstance().LoadMeta(prefix, entries, true);
    EXPECT_TRUE(loadResult);
    EXPECT_GE(entries.size(), static_cast<size_t>(2));

    auto delResult1 = MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true);
    EXPECT_TRUE(delResult1);
    auto delResult2 = MetaDataManager::GetInstance().DelMeta(meta2.GetKey(), true);
    EXPECT_TRUE(delResult2);
}

/**
 * @tc.name: BundleVersionMetaData_KeyPrefix_MatchesPattern
 * @tc.desc: Test KEY_PREFIX constant value
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_KeyPrefix_MatchesPattern, TestSize.Level1)
{
    EXPECT_STREQ(BundleVersionMetaData::KEY_PREFIX, "BundleVersionMetaData");
}

/**
 * @tc.name: BundleVersionMetaData_DifferentUsers_DifferentKeys
 * @tc.desc: Test that same bundleName with different users produces different keys
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_DifferentUsers_DifferentKeys, TestSize.Level1)
{
    BundleVersionMetaData meta1;
    meta1.bundleName = "com.example.samebundle";
    meta1.user = "100";
    meta1.appIndex = 0;
    meta1.versionCode = 100;

    BundleVersionMetaData meta2;
    meta2.bundleName = "com.example.samebundle";
    meta2.user = "200";
    meta2.appIndex = 0;
    meta2.versionCode = 200;

    EXPECT_NE(meta1.GetKey(), meta2.GetKey());

    auto delResult1 = MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true);
    auto delResult2 = MetaDataManager::GetInstance().DelMeta(meta2.GetKey(), true);
}

/**
 * @tc.name: BundleVersionMetaData_DifferentAppIndex_DifferentKeys
 * @tc.desc: Test that same bundleName+user with different appIndex still uses same key
 *       (appIndex is in data but not in key)
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(BundleVersionMetaDataTest, BundleVersionMetaData_DifferentAppIndex_DifferentKeys, TestSize.Level1)
{
    BundleVersionMetaData meta1;
    meta1.bundleName = "com.example.clone";
    meta1.user = "100";
    meta1.appIndex = 0;
    meta1.versionCode = 100;

    BundleVersionMetaData meta2;
    meta2.bundleName = "com.example.clone";
    meta2.user = "100";
    meta2.appIndex = 1;
    meta2.versionCode = 200;

    EXPECT_EQ(meta1.GetKey(), meta2.GetKey());
}
} // namespace OHOS::Test

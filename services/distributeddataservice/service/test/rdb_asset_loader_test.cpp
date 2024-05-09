/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "RdbAssetLoaderTest"

#include <gmock/gmock.h>
#include "gtest/gtest.h"
#include "log_print.h"
#include "rdb_asset_loader.h"
#include "store/cursor.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using Type = DistributedDB::Type;
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbAssetLoaderTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class MockAssetLoader : public DistributedData::AssetLoader {
public:
    MOCK_METHOD4(Download, int32_t(const std::string &, const std::string &,
        const DistributedData::Value &, VBucket &));
    MOCK_METHOD4(RemoveLocalAssets, int32_t(const std::string &, const std::string &,
        const DistributedData::Value &, VBucket &));
};

/**
* @tc.name: Download
* @tc.desc: RdbAssetLoader download test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, Download, TestSize.Level0)
{
    BindAssets bindAssets;
    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::string tableName = "testTable";
    std::string groupId = "testGroup";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    EXPECT_CALL(*cloudAssetLoader,
        Download(tableName, groupId, _, _)).WillRepeatedly(testing::Return(GeneralError::E_OK));
    auto result = rdbAssetLoader.Download(tableName, groupId, prefix, assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: RemoveLocalAssets
* @tc.desc: RdbAssetLoader RemoveLocalAssets test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, RemoveLocalAssets, TestSize.Level0)
{
    BindAssets bindAssets;
    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::vector<DistributedDB::Asset> assets;
    EXPECT_CALL(*cloudAssetLoader, RemoveLocalAssets(_, _, _, _)).WillRepeatedly(testing::Return(GeneralError::E_OK));
    auto result = rdbAssetLoader.RemoveLocalAssets(assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: PostEvent
* @tc.desc: RdbAssetLoader PostEvent abnormal
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent, TestSize.Level0)
{
    BindAssets bindAssets;
    bindAssets.bindAssets = nullptr;
    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::string tableName = "testTable";
    std::string groupId = "testGroup";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    EXPECT_CALL(*cloudAssetLoader, Download(tableName, groupId, _, _)).WillRepeatedly(testing::Return(E_NOT_SUPPORT));
    auto result = rdbAssetLoader.Download(tableName, groupId, prefix, assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::CLOUD_ERROR);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
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
#include "extension_util.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::CloudData;
 
namespace OHOS::Test {
class ExtensionUtilTest : public testing::Test {};

/**
* @tc.name: Convert001
* @tc.desc: Check that the character string does not contain null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert001, TestSize.Level1)
{
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = "http://example.com/path/to/file";
    dbAsset.path = "/path/to/file";

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_NE(asset, nullptr);
    EXPECT_GT(assetLen, 0);
    delete asset;
}

/**
* @tc.name: Convert002
* @tc.desc: Check whether the path contains null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert002, TestSize.Level1)
{
    const char data[] = "/path\0/to/file";
    std::string path(data, 13);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = "http://example.com/path/to/file";
    dbAsset.path = path;

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: Convert003
* @tc.desc: Check whether the URI contains null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert003, TestSize.Level1)
{
    const char data[] = "http://example.com/path\0to/file";
    std::string uri(data, 32);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = uri;
    dbAsset.path = "/path/to/file";

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: Convert004
* @tc.desc: Check whether the path and URI contain null characters.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, Convert004, TestSize.Level1)
{
    const char data1[] = "http://example.com/path\0to/file";
    std::string uri(data1, 32);
    const char data2[] = "/path\0/to/file";
    std::string path(data2, 13);
    DBAsset dbAsset;
    dbAsset.version = 1;
    dbAsset.status = DBAsset::STATUS_NORMAL;
    dbAsset.expiresTime = 1234567890;
    dbAsset.id = "12345";
    dbAsset.name = "test_asset";
    dbAsset.createTime = "2025-07-03T00:00:00Z";
    dbAsset.modifyTime = "2025-07-04T00:00:00Z";
    dbAsset.size = "1024";
    dbAsset.hash = "abc123";
    dbAsset.uri = uri;
    dbAsset.path = path;

    auto [asset, assetLen] = ExtensionUtil::Convert(dbAsset);
    EXPECT_EQ(asset, nullptr);
    EXPECT_EQ(assetLen, 0);
}

/**
* @tc.name: ConvertVBuckets001
* @tc.desc: Test that convert DBVBuckets to empty.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets001, TestSize.Level1)
{
    DBVBuckets buckets;
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_NE(result, nullptr);
    EXPECT_EQ(size, 0);
}

/**
* @tc.name: ConvertVBuckets002
* @tc.desc: Test that convert function DBVBucket is DBAsset type.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets002, TestSize.Level1)
{
    DBVBuckets buckets;
    DBVBucket bucket;
    DBAsset dbAsset;
    bucket["dbAsset"] = dbAsset;
    buckets.push_back(bucket);
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_NE(result, nullptr);
    EXPECT_GT(size, 0);
}

/**
* @tc.name: ConvertVBuckets003
* @tc.desc: Test that convert function DBVBucket is DBAssets type.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets003, TestSize.Level1)
{
    DBVBuckets buckets;
    DBVBucket bucket;
    DBAssets dbAssets;
    bucket["dbAsset"] = dbAssets;
    buckets.push_back(bucket);
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_NE(result, nullptr);
    EXPECT_GT(size, 0);
}

/**
* @tc.name: ConvertVBuckets004
* @tc.desc: Test that convert function DBVBucket is str type.
* @tc.type: FUNC
*/
HWTEST_F(ExtensionUtilTest, ConvertVBuckets004, TestSize.Level1)
{
    DBVBuckets buckets;
    DBVBucket bucket;
    std::string key = "123";
    bucket["dbAsset"] = key;
    buckets.push_back(bucket);
    auto [result, size] = ExtensionUtil::Convert(std::move(buckets));
    EXPECT_NE(result, nullptr);
    EXPECT_GT(size, 0);
}
} // namespace OHOS::Test
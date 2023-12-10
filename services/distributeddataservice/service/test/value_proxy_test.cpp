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

#define LOG_TAG "CloudDataTest"
#include <gtest/gtest.h>
#include "log_print.h"
#include "value_proxy.h"
namespace Test {
using namespace testing::ext;
using namespace OHOS::DistributedData;
class ValueProxyTest  : public testing::Test {
};

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, VBucketsNormal2GaussDB, TestSize.Level0)
{
    std::vector<DistributedDB::VBucket> dbVBuckets;
    OHOS::DistributedData::VBuckets extends = {
        {{"#gid", {"0000000"}}, {"#flag", {true }}, {"#value", {int64_t(100)}}, {"#float", {double(100)}}},
        {{"#gid", {"0000001"}}}
    };
    dbVBuckets = ValueProxy::Convert(std::move(extends));
    ASSERT_EQ(dbVBuckets.size(), 2);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, VBucketsGaussDB2Normal, TestSize.Level0)
{
    std::vector<DistributedDB::VBucket> dbVBuckets = {
        {{"#gid", {"0000000"}}, {"#flag", {true }}, {"#value", {int64_t(100)}}, {"#float", {double(100)}}},
        {{"#gid", {"0000001"}}}
    };
    OHOS::DistributedData::VBuckets extends;
    extends = ValueProxy::Convert(std::move(dbVBuckets));
    ASSERT_EQ(extends.size(), 2);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, VBucketsNormal2Rdb, TestSize.Level0)
{
    using RdbBucket = OHOS::NativeRdb::ValuesBucket;
    std::vector<RdbBucket> rdbVBuckets;
    OHOS::DistributedData::VBuckets extends = {
        {{"#gid", {"0000000"}}, {"#flag", {true }}, {"#value", {int64_t(100)}}, {"#float", {double(100)}}},
        {{"#gid", {"0000001"}}}
    };
    rdbVBuckets = ValueProxy::Convert(std::move(extends));
    ASSERT_EQ(rdbVBuckets.size(), 2);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, VBucketsRdb2Normal, TestSize.Level0)
{
    using RdbBucket = OHOS::NativeRdb::ValuesBucket;
    using RdbValue = OHOS::NativeRdb::ValueObject;
    std::vector<RdbBucket> rdbVBuckets = {
        RdbBucket(std::map<std::string, RdbValue> {
            {"#gid", {"0000000"}},
            {"#flag", {true }},
            {"#value", {int64_t(100)}},
            {"#float", {double(100)}}
        }),
        RdbBucket(std::map<std::string, RdbValue> {
            {"#gid", {"0000001"}}
        })
    };
    OHOS::DistributedData::VBuckets extends;
    extends = ValueProxy::Convert(std::move(rdbVBuckets));
    ASSERT_EQ(extends.size(), 2);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, ConvertIntMapTest, TestSize.Level0)
{
    std::map<std::string, int64_t> testMap = { { "name", 1 }, { "school", 2 }, { "address", 3 } };
    auto res = ValueProxy::Convert<int64_t>(testMap);
    auto testMap2 = std::map<std::string, int64_t>(res);
    ASSERT_EQ(testMap2.find("name")->second, 1);

    auto errorMap = std::map<std::string, double>(res);
    ASSERT_EQ(errorMap.size(), 0);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, ConvertAssetMapGaussDB2NormalTest, TestSize.Level0)
{
    DistributedDB::Asset dbAsset0 { .name = "dbname", .uri = "dburi" };
    DistributedDB::Asset dbAsset1 { .name = "dbname", .uri = "dburi" };
    std::map<std::string, DistributedDB::Asset> dbMap { { "asset0", dbAsset0 }, { "asset1", dbAsset1 } };
    OHOS::DistributedData::VBucket transferredAsset = ValueProxy::Convert(dbMap);
    ASSERT_EQ(transferredAsset.size(), 2);
    auto asset = std::get<OHOS::DistributedData::Asset>(transferredAsset.find("asset0")->second);
    ASSERT_EQ(asset.name, "dbname");

    DistributedDB::Assets dbAssets { dbAsset0, dbAsset1 };
    std::map<std::string, DistributedDB::Assets> dbAssetsMap { {"dbAssets", dbAssets} };
    OHOS::DistributedData::VBucket transferredAssets = ValueProxy::Convert(dbAssetsMap);
    ASSERT_EQ(transferredAssets.size(), 1);
    auto assets = std::get<OHOS::DistributedData::Assets>(transferredAssets.find("dbAssets")->second);
    ASSERT_EQ(assets.size(), 2);
    auto dataAsset = assets.begin();
    ASSERT_EQ(dataAsset->name, "dbname");
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, ConvertAssetMapNormal2GaussDBTest, TestSize.Level0)
{
    using NormalAsset = OHOS::DistributedData::Asset;
    using NormalAssets = OHOS::DistributedData::Assets;
    NormalAsset nAsset0 { .name = "name", .uri = "uri" };
    NormalAsset nAsset1 { .name = "name", .uri = "uri" };
    std::map<std::string, NormalAsset> nMap { { "asset0", nAsset0 }, { "asset1", nAsset1 } };
    DistributedDB::VBucket transferredAsset = ValueProxy::Convert(nMap);
    ASSERT_EQ(transferredAsset.size(), 2);
    auto asset = std::get<DistributedDB::Asset>(transferredAsset.find("asset0")->second);
    ASSERT_EQ(asset.name, "name");

    NormalAssets nAssets { nAsset0, nAsset1 };
    std::map<std::string, NormalAssets> nAssetsMap { { "Assets", nAssets } };
    DistributedDB::VBucket transferredAssets = ValueProxy::Convert(nAssetsMap);
    ASSERT_EQ(transferredAssets.size(), 1);
    auto assets = std::get<DistributedDB::Assets>(transferredAssets.find("Assets")->second);
    ASSERT_EQ(assets.size(), 2);
    auto dataAsset = assets.begin();
    ASSERT_EQ(dataAsset->name, "name");
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, ConvertAssetMapRdb2NormalTest, TestSize.Level0)
{
    using RdbAsset = OHOS::NativeRdb::AssetValue;
    using RdbAssets = std::vector<RdbAsset>;
    RdbAsset dbAsset0 { .name = "dbname", .uri = "dburi" };
    RdbAsset dbAsset1 { .name = "dbname", .uri = "dburi" };
    std::map<std::string, RdbAsset> dbMap { { "asset0", dbAsset0 }, { "asset1", dbAsset1 } };
    OHOS::DistributedData::VBucket transferredAsset = ValueProxy::Convert(dbMap);
    ASSERT_EQ(transferredAsset.size(), 2);
    auto asset = std::get<OHOS::DistributedData::Asset>(transferredAsset.find("asset0")->second);
    ASSERT_EQ(asset.name, "dbname");

    RdbAssets dbAssets { dbAsset0, dbAsset1 };
    std::map<std::string, RdbAssets> dbAssetsMap { {"dbAssets", dbAssets} };
    OHOS::DistributedData::VBucket transferredAssets = ValueProxy::Convert(dbAssetsMap);
    ASSERT_EQ(transferredAssets.size(), 1);
    auto assets = std::get<OHOS::DistributedData::Assets>(transferredAssets.find("dbAssets")->second);
    ASSERT_EQ(assets.size(), 2);
    auto dataAsset = assets.begin();
    ASSERT_EQ(dataAsset->name, "dbname");
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from cloud when no schema in meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(ValueProxyTest, ConvertAssetMapNormal2RdbTest, TestSize.Level0)
{
    using RdbAsset = OHOS::NativeRdb::AssetValue;
    using RdbAssets = std::vector<RdbAsset>;
    using NormalAsset = OHOS::DistributedData::Asset;
    using NormalAssets = OHOS::DistributedData::Assets;
    NormalAsset nAsset0 { .name = "name", .uri = "uri" };
    NormalAsset nAsset1 { .name = "name", .uri = "uri" };
    std::map<std::string, NormalAsset> nMap { { "asset0", nAsset0 }, { "asset1", nAsset1 } };
    OHOS::NativeRdb::ValuesBucket transferredAsset = ValueProxy::Convert(nMap);
    ASSERT_EQ(transferredAsset.Size(), 2);
    OHOS::NativeRdb::ValueObject rdbObject;
    transferredAsset.GetObject("asset0", rdbObject);
    RdbAsset rdbAsset;
    rdbObject.GetAsset(rdbAsset);
    ASSERT_EQ(rdbAsset.name, "name");

    NormalAssets nAssets { nAsset0, nAsset1 };
    std::map<std::string, NormalAssets> nAssetsMap { { "Assets", nAssets } };
    OHOS::NativeRdb::ValuesBucket transferredAssets = ValueProxy::Convert(nAssetsMap);
    ASSERT_EQ(transferredAssets.Size(), 1);
    OHOS::NativeRdb::ValueObject rdbObject2;
    transferredAssets.GetObject("Assets", rdbObject2);
    RdbAssets rdbAssets;
    rdbObject2.GetAssets(rdbAssets);
    ASSERT_EQ(rdbAssets.size(), 2);
    auto dataAsset = rdbAssets.begin();
    ASSERT_EQ(dataAsset->name, "name");
}
} // namespace Test

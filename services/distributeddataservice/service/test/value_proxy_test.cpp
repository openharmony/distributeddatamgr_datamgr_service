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
using namespace OHOS::DistributedRdb;
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
}

/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "CloudServiceStubTest"

#include <gtest/gtest.h>

#include "cloud_service_impl.h"
#include "cloud_service_stub.h"
#include "cloud_types_util.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::CloudData;
using CloudServiceStub = OHOS::CloudData::CloudServiceStub;

namespace OHOS::ITypesUtil {
template<>
bool Marshalling(const BundleInfo &input, MessageParcel &data)
{
    return Marshal(data, input.bundleName, input.storeId);
}

template<>
bool Marshalling(const CloudSubscribeType &type, MessageParcel &data)
{
    return Marshal(data, static_cast<uint32_t>(type));
}
}
namespace OHOS::Test {
namespace CloudDataTest {
class CloudServiceStubTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;


/**
 * @tc.name: OnQueryLastSyncInfoBatch_EmptyId
 * @tc.desc: Test OnQueryLastSyncInfoBatch with empty id
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnQueryLastSyncInfoBatch_EmptyId, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    std::string id = "";
    std::vector<BundleInfo> bundleInfos;
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, id, bundleInfos));
    
    auto result = cloudServiceStub->OnQueryLastSyncInfoBatch(data, reply);
    EXPECT_EQ(result, ERR_NONE);
    auto status = reply.ReadInt32();
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
}

/**
 * @tc.name: OnQueryLastSyncInfoBatch_EmptyBundleInfos
 * @tc.desc: Test OnQueryLastSyncInfoBatch with empty bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnQueryLastSyncInfoBatch_EmptyBundleInfos, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    std::string id = "test_account_id";
    std::vector<BundleInfo> bundleInfos;
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, id, bundleInfos));
    
    auto result = cloudServiceStub->OnQueryLastSyncInfoBatch(data, reply);
    EXPECT_EQ(result, ERR_NONE);
    auto status = reply.ReadInt32();
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
}

/**
 * @tc.name: OnQueryLastSyncInfoBatch_ExceedMaxLimit
 * @tc.desc: Test OnQueryLastSyncInfoBatch with bundleInfos exceeding maximum limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnQueryLastSyncInfoBatch_ExceedMaxLimit, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    std::string id = "test_account_id";
    std::vector<BundleInfo> bundleInfos;
    for (size_t i = 0; i < 31; ++i) {
        BundleInfo info;
        info.bundleName = "bundle_" + std::to_string(i);
        info.storeId = "store_" + std::to_string(i);
        bundleInfos.push_back(info);
    }
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, id, bundleInfos));
    
    auto result = cloudServiceStub->OnQueryLastSyncInfoBatch(data, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnQueryLastSyncInfoBatch_UnmarshalFailed
 * @tc.desc: Test OnQueryLastSyncInfoBatch with unmarshal failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnQueryLastSyncInfoBatch_UnmarshalFailed, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    
    auto result = cloudServiceStub->OnQueryLastSyncInfoBatch(data, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnSubscribe_EmptyBundleInfos
 * @tc.desc: Test OnSubscribe with empty bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnSubscribe_EmptyBundleInfos, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    CloudSubscribeType type = CloudSubscribeType::SYNC_INFO_CHANGED;
    std::vector<BundleInfo> bundleInfos;
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, type, bundleInfos));
    
    auto result = cloudServiceStub->OnSubscribe(data, reply);
    EXPECT_EQ(result, ERR_NONE);
    auto status = reply.ReadInt32();
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
}

/**
 * @tc.name: OnSubscribe_UnmarshalFailed
 * @tc.desc: Test OnSubscribe with unmarshal failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnSubscribe_UnmarshalFailed, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    CloudSubscribeType type = CloudSubscribeType::SYNC_INFO_CHANGED;
    std::vector<BundleInfo> bundleInfos;
    for (size_t i = 0; i < 31; ++i) {
        BundleInfo info;
        info.bundleName = "bundle_" + std::to_string(i);
        info.storeId = "store_" + std::to_string(i);
        bundleInfos.push_back(info);
    }
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, type, bundleInfos));
    
    auto result = cloudServiceStub->OnSubscribe(data, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnSubscribe_ExceedMaxLimit
 * @tc.desc: Test OnSubscribe with bundleInfos exceeding maximum limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnSubscribe_ExceedMaxLimit, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    
    auto result = cloudServiceStub->OnSubscribe(data, reply);
    EXPECT_EQ(result, CloudService::IPC_PARCEL_ERROR);
}

/**
 * @tc.name: OnUnsubscribe_EmptyBundleInfos
 * @tc.desc: Test OnUnsubscribe with empty bundleInfos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnUnsubscribe_EmptyBundleInfos, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    CloudSubscribeType type = CloudSubscribeType::SYNC_INFO_CHANGED;
    std::vector<BundleInfo> bundleInfos;
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, type, bundleInfos));
    
    auto result = cloudServiceStub->OnUnsubscribe(data, reply);
    EXPECT_EQ(result, ERR_NONE);
    auto status = reply.ReadInt32();
    EXPECT_EQ(status, CloudData::CloudService::INVALID_ARGUMENT_V20);
}

/**
 * @tc.name: OnUnsubscribe_ExceedMaxLimit
 * @tc.desc: Test OnUnsubscribe with bundleInfos exceeding maximum limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnUnsubscribe_ExceedMaxLimit, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    CloudSubscribeType type = CloudSubscribeType::SYNC_INFO_CHANGED;
    std::vector<BundleInfo> bundleInfos;
    for (size_t i = 0; i < 31; ++i) {
        BundleInfo info;
        info.bundleName = "bundle_" + std::to_string(i);
        info.storeId = "store_" + std::to_string(i);
        bundleInfos.push_back(info);
    }
    
    ASSERT_TRUE(ITypesUtil::Marshal(data, type, bundleInfos));
    
    auto result = cloudServiceStub->OnUnsubscribe(data, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}

/**
 * @tc.name: OnUnsubscribe_UnmarshalFailed
 * @tc.desc: Test OnUnsubscribe with unmarshal failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudServiceStubTest, OnUnsubscribe_UnmarshalFailed, TestSize.Level1)
{
    MessageParcel data;
    MessageParcel reply;
    
    auto result = cloudServiceStub->OnUnsubscribe(data, reply);
    EXPECT_EQ(result, CloudService::IPC_PARCEL_ERROR);
}
} // namespace CloudDataTest
} // namespace OHOS::Test

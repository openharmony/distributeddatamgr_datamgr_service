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
#define LOG_TAG "CloudNotifierProxyTest"

#include <gtest/gtest.h>

#include "cloud_notifier_proxy.h"
#include "cloud_service.h"
#include "cloud_types.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::CloudData;

namespace OHOS::Test {
namespace CloudNotifierProxyTest {

class RemoteObjectTest : public IRemoteObject {
public:
    explicit RemoteObjectTest(std::u16string descriptor) : IRemoteObject(descriptor) {}
    virtual ~RemoteObjectTest() = default;

    int32_t GetObjectRefCount() override
    {
        return 0;
    }

    int32_t SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return CloudService::SUCCESS;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    sptr<IRemoteBroker> AsInterface() override
    {
        return nullptr;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        return 0;
    }
};

class RemoteObjectErrorTest : public IRemoteObject {
public:
    explicit RemoteObjectErrorTest(std::u16string descriptor) : IRemoteObject(descriptor) {}
    virtual ~RemoteObjectErrorTest() = default;

    int32_t GetObjectRefCount() override
    {
        return 0;
    }

    int32_t SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return CloudService::IPC_ERROR;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    sptr<IRemoteBroker> AsInterface() override
    {
        return nullptr;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        return 0;
    }
};

class CloudNotifierProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
 * @tc.name: OnSyncInfoNotify_SendRequestSuccess
 * @tc.desc: Test OnSyncInfoNotify with successful send request
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_SendRequestSuccess, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    QueryLastResults innerData;
    CloudSyncInfo syncInfo;
    syncInfo.startTime = 1000;
    syncInfo.finishTime = 2000;
    syncInfo.code = 0;
    syncInfo.syncStatus = SyncStatus::FINISHED;
    innerData["store1"] = syncInfo;
    data["bundle1"] = innerData;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}

/**
 * @tc.name: OnSyncInfoNotify_EmptyData
 * @tc.desc: Test OnSyncInfoNotify with empty data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_EmptyData, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}

/**
 * @tc.name: OnSyncInfoNotify_RemoteNull
 * @tc.desc: Test OnSyncInfoNotify with null remote object
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_RemoteNull, TestSize.Level1)
{
    CloudNotifierProxy proxy(nullptr);
    
    BatchQueryLastResults data;
    QueryLastResults innerData;
    CloudSyncInfo syncInfo;
    syncInfo.startTime = 1000;
    syncInfo.finishTime = 2000;
    syncInfo.code = 0;
    syncInfo.syncStatus = SyncStatus::FINISHED;
    innerData["store1"] = syncInfo;
    data["bundle1"] = innerData;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::IPC_ERROR);
}

/**
 * @tc.name: OnSyncInfoNotify_SendRequestFailed
 * @tc.desc: Test OnSyncInfoNotify with send request failed
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_SendRequestFailed, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectErrorTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    QueryLastResults innerData;
    CloudSyncInfo syncInfo;
    syncInfo.startTime = 1000;
    syncInfo.finishTime = 2000;
    syncInfo.code = 0;
    syncInfo.syncStatus = SyncStatus::FINISHED;
    innerData["store1"] = syncInfo;
    data["bundle1"] = innerData;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::IPC_ERROR);
}

/**
 * @tc.name: OnSyncInfoNotify_MultipleBundles
 * @tc.desc: Test OnSyncInfoNotify with multiple bundles
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_MultipleBundles, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    
    QueryLastResults bundle1Data;
    CloudSyncInfo syncInfo1;
    syncInfo1.startTime = 1000;
    syncInfo1.finishTime = 2000;
    syncInfo1.code = 0;
    syncInfo1.syncStatus = SyncStatus::FINISHED;
    bundle1Data["store1"] = syncInfo1;
    
    CloudSyncInfo syncInfo2;
    syncInfo2.startTime = 1001;
    syncInfo2.finishTime = 2001;
    syncInfo2.code = 1;
    syncInfo2.syncStatus = SyncStatus::RUNNING;
    bundle1Data["store2"] = syncInfo2;
    data["bundle1"] = bundle1Data;
    
    QueryLastResults bundle2Data;
    CloudSyncInfo syncInfo3;
    syncInfo3.startTime = 1002;
    syncInfo3.finishTime = 2002;
    syncInfo3.code = 0;
    syncInfo3.syncStatus = SyncStatus::FINISHED;
    bundle2Data["store3"] = syncInfo3;
    data["bundle2"] = bundle2Data;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}

/**
 * @tc.name: OnSyncInfoNotify_DifferentSyncStatus
 * @tc.desc: Test OnSyncInfoNotify with different sync statuses
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_DifferentSyncStatus, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    
    QueryLastResults runningData;
    CloudSyncInfo runningInfo;
    runningInfo.startTime = 1000;
    runningInfo.finishTime = 0;
    runningInfo.code = -1;
    runningInfo.syncStatus = SyncStatus::RUNNING;
    runningData["store1"] = runningInfo;
    data["bundle1"] = runningData;
    
    QueryLastResults finishedData;
    CloudSyncInfo finishedInfo;
    finishedInfo.startTime = 1000;
    finishedInfo.finishTime = 2000;
    finishedInfo.code = 0;
    finishedInfo.syncStatus = SyncStatus::FINISHED;
    finishedData["store2"] = finishedInfo;
    data["bundle2"] = finishedData;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}

/**
 * @tc.name: OnSyncInfoNotify_ErrorCodes
 * @tc.desc: Test OnSyncInfoNotify with various error codes in CloudSyncInfo
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_ErrorCodes, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    
    QueryLastResults errorData;
    
    CloudSyncInfo info1;
    info1.startTime = 1000;
    info1.finishTime = 2000;
    info1.code = 1;
    info1.syncStatus = SyncStatus::FINISHED;
    errorData["store1"] = info1;
    
    CloudSyncInfo info2;
    info2.startTime = 1001;
    info2.finishTime = 2001;
    info2.code = -1;
    info2.syncStatus = SyncStatus::RUNNING;
    errorData["store2"] = info2;
    
    CloudSyncInfo info3;
    info3.startTime = 1002;
    info3.finishTime = 2002;
    info3.code = 100;
    info3.syncStatus = SyncStatus::FINISHED;
    errorData["store3"] = info3;
    
    data["bundle1"] = errorData;
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}

/**
 * @tc.name: OnSyncInfoNotify_LargeData
 * @tc.desc: Test OnSyncInfoNotify with large amount of data
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(CloudNotifierProxyTest, OnSyncInfoNotify_LargeData, TestSize.Level1)
{
    std::u16string descriptor = u"OHOS.CloudData.ICloudNotifier";
    sptr<IRemoteObject> remote = new (std::nothrow) RemoteObjectTest(descriptor);
    ASSERT_NE(remote, nullptr);
    
    CloudNotifierProxy proxy(remote);
    
    BatchQueryLastResults data;
    
    for (int i = 0; i < 10; ++i) {
        QueryLastResults bundleData;
        for (int j = 0; j < 5; ++j) {
            CloudSyncInfo info;
            info.startTime = 1000 + i * 100 + j;
            info.finishTime = 2000 + i * 100 + j;
            info.code = j % 5;
            info.syncStatus = (j % 2 == 0) ? SyncStatus::FINISHED : SyncStatus::RUNNING;
            bundleData["store_" + std::to_string(i) + "_" + std::to_string(j)] = info;
        }
        data["bundle_" + std::to_string(i)] = bundleData;
    }
    
    auto result = proxy.OnSyncInfoNotify(data);
    EXPECT_EQ(result, CloudService::SUCCESS);
}
} // namespace CloudNotifierProxyTest
} // namespace OHOS::Test


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

#define LOG_TAG "ObjectAssetSyncManagerTest"
#include <dlfcn.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>

#include "asset_sync_manager.h"
#include "cloud_sync_asset_manager.h"
#include "executor_pool.h"
#include "object_asset_loader.h"
#include "object_common.h"
#include "snapshot/machine_status.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace OHOS::FileManagement::CloudSync;
using ::testing::NiceMock;

class MockCloudSyncAssetManager : public CloudSyncAssetManager {
public:
    MOCK_METHOD(int32_t, UploadAsset, (int32_t, const std::string &, std::string &), (override));
    MOCK_METHOD(int32_t, DownloadFile, (int32_t, const std::string &, AssetInfo &), (override));
    MOCK_METHOD(int32_t, DownloadFiles,
        (int32_t, const std::string &, const std::vector<AssetInfo> &, std::vector<bool> &, int32_t), (override));
    MOCK_METHOD(int32_t, DeleteAsset, (int32_t, const std::string &), (override));

    enum class CallbackType { SUCCESS, DIRECT_FAILURE, CALLBACK_FAILURE, TIMEOUT };

    static void SetCallbackType(CallbackType type)
    {
        callbackType_ = type;
    }
    int32_t DownloadFile(const int32_t userId, const std::string &bundleName, const std::string &networkId,
        AssetInfo &assetInfo, ResultCallback resultCallback) override
    {
        (void)userId;
        (void)bundleName;
        (void)networkId;
        (void)assetInfo;
        switch (callbackType_) {
            case CallbackType::SUCCESS:
                resultCallback("", OBJECT_SUCCESS);
                return OBJECT_SUCCESS;
            case CallbackType::DIRECT_FAILURE:
                return OBJECT_INNER_ERROR;
            case CallbackType::CALLBACK_FAILURE:
                resultCallback("", OBJECT_INNER_ERROR);
                return OBJECT_INNER_ERROR;
            case CallbackType::TIMEOUT:
                return OBJECT_SUCCESS;
            default:
                break;
        }
        return OBJECT_INNER_ERROR;
    }

private:
    static inline CallbackType callbackType_ = CallbackType::SUCCESS;
};
namespace OHOS::FileManagement::CloudSync {
CloudSyncAssetManager &CloudSyncAssetManager::GetInstance()
{
    static MockCloudSyncAssetManager instance;
    return instance;
}
}

namespace OHOS::Test {
class MockDlsym {
public:
    MOCK_METHOD(void *, dlopen, (const char *fileName, int flag));
};

NiceMock<MockDlsym> *mockDlsym;

extern "C" {
// mock dlopen
void *dlopen(const char *fileName, int flag)
{
    if (mockDlsym == nullptr) {
        mockDlsym = new NiceMock<MockDlsym>();
    }
    return mockDlsym->dlopen(fileName, flag);
}
}

class ObjectAssetSyncManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

protected:
    Asset asset_;
    std::string uri_;
    int32_t userId_ = 0;
    std::string bundleName_ = "test_bundleName_1";
    std::string deviceId_ = "test_deviceId__1";
};

void ObjectAssetSyncManagerTest::SetUpTestCase(void)
{
    mockDlsym = new NiceMock<MockDlsym>();
}

void ObjectAssetSyncManagerTest::TearDownTestCase(void)
{
    delete mockDlsym;
    mockDlsym = nullptr;
}
void ObjectAssetSyncManagerTest::SetUp()
{
    uri_ = "file:://com.example.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    asset_ = asset;
}

void ObjectAssetSyncManagerTest::TearDown()
{
}

/**
 * @tc.name: Transfer001
 * @tc.desc: dlopen is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ObjectAssetSyncManagerTest, UploadTest001, TestSize.Level0)
{
    auto &assetLoader = ObjectAssetLoader::GetInstance();
    EXPECT_CALL(*mockDlsym, dlopen(::testing::_, ::testing::_)).WillOnce(Return(nullptr));
    auto result = assetLoader.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: UploadTest002
 * @tc.desc: dlsym return null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(ObjectAssetSyncManagerTest, UploadTest002, TestSize.Level0)
{
    auto &assetLoader = ObjectAssetLoader::GetInstance();
    void *handle = reinterpret_cast<void *>(0x123456);
    EXPECT_CALL(*mockDlsym, dlopen(::testing::_, ::testing::_)).WillOnce(Return(handle));
    auto result = assetLoader.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: TransferAsset_Success
 * @tc.desc: Should succeed when download returns success immediately.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectAssetSyncManagerTest, TransferAsset_Success, TestSize.Level0)
{
    AssetSyncManager assetSyncManager;
    MockCloudSyncAssetManager::SetCallbackType(MockCloudSyncAssetManager::CallbackType::SUCCESS);
    auto result = assetSyncManager.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: TransferAsset_DirectFailure
 * @tc.desc: Should fail when DownloadFile returns error directly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectAssetSyncManagerTest, TransferAsset_DirectFailure, TestSize.Level0)
{
    AssetSyncManager assetSyncManager;
    MockCloudSyncAssetManager::SetCallbackType(MockCloudSyncAssetManager::CallbackType::DIRECT_FAILURE);
    auto result = assetSyncManager.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: TransferAsset_CallbackFailure
 * @tc.desc: Should fail when download succeeds but callback returns error.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectAssetSyncManagerTest, TransferAsset_CallbackFailure, TestSize.Level0)
{
    AssetSyncManager assetSyncManager;
    MockCloudSyncAssetManager::SetCallbackType(MockCloudSyncAssetManager::CallbackType::CALLBACK_FAILURE);
    auto result = assetSyncManager.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: TransferAsset_Timeout
 * @tc.desc: Should timeout when callback never called.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectAssetSyncManagerTest, TransferAsset_Timeout, TestSize.Level1)
{
    AssetSyncManager assetSyncManager;
    MockCloudSyncAssetManager::SetCallbackType(MockCloudSyncAssetManager::CallbackType::TIMEOUT);
    auto result = assetSyncManager.Transfer(userId_, bundleName_, deviceId_, asset_);
    EXPECT_FALSE(result);
}
} // namespace OHOS::Test

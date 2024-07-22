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

#define LOG_TAG "ObjectAssetLoaderTest"

#include "object_asset_loader.h"
#include <gtest/gtest.h>
#include "snapshot/machine_status.h"
#include "executor_pool.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
namespace OHOS::Test {

class ObjectAssetLoaderTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    Asset asset_;
    std::string uri_;
    int32_t userId_ = 1;
    std::string bundleName_ = "test_bundleName_1";
    std::string deviceId_ = "test_deviceId__1";
};

void ObjectAssetLoaderTest::SetUp()
{
    uri_ = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    asset_ = asset;
}

void ObjectAssetLoaderTest::TearDown() {}

/**
* @tc.name: UploadTest001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, UploadTest001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    auto result = assetLoader->Transfer(userId_, bundleName_, deviceId_, asset_);
    ASSERT_EQ(result, false);
}

/**
* @tc.name: TransferAssetsAsync001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, TransferAssetsAsync001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    std::function<void(bool)> lambdaFunc = [](bool success) {
        if (success) {}
    };
    std::vector<Asset> assets{ asset_ };
    ASSERT_EQ(assetLoader->executors_, nullptr);
    assetLoader->TransferAssetsAsync(userId_, bundleName_, deviceId_, assets, lambdaFunc);
}

/**
* @tc.name: TransferAssetsAsync002
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, TransferAssetsAsync002, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    std::function<void(bool)> lambdaFunc = [](bool success) {
        if (success) {}
    };
    std::vector<Asset> assets{ asset_ };
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(100, 0);
    ASSERT_NE(executors, nullptr);
    assetLoader->SetThreadPool(executors);
    ASSERT_NE(assetLoader->executors_, nullptr);
    assetLoader->TransferAssetsAsync(userId_, bundleName_, deviceId_, assets, lambdaFunc);
}

/**
* @tc.name: FinishTask001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, FinishTask001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    assetLoader->FinishTask(asset_.uri, false);
    assetLoader->FinishTask(asset_.uri, true);
}

/**
* @tc.name: IsDownloading001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, IsDownloading001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    assetLoader->IsDownloading(asset_);
    assetLoader->UpdateDownloaded(asset_);
    assetLoader->IsDownloading(asset_);
}

/**
* @tc.name: IsDownloaded001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, IsDownloaded001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    assetLoader->IsDownloaded(asset_);
    assetLoader->UpdateDownloaded(asset_);
    assetLoader->IsDownloaded(asset_);
}

/**
* @tc.name: UpdateDownloaded001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectAssetLoaderTest, UpdateDownloaded001, TestSize.Level0)
{
    auto assetLoader = ObjectAssetLoader::GetInstance();
    assetLoader->UpdateDownloaded(asset_);
    assetLoader->UpdateDownloaded(asset_);
    for (int i = 0; i <= assetLoader->LAST_DOWNLOAD_ASSET_SIZE; i++) {
        assetLoader->assetQueue_.push(asset_.uri);
    }
    assetLoader->UpdateDownloaded(asset_);
}
} // namespace OHOS::Test
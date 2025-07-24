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

#define LOG_TAG "ObjectServiceImplTest"

#include "object_service_impl.h"

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "device_manager_adapter_mock.h"
#include "ipc_skeleton.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace std;
using namespace testing;
namespace OHOS::Test {
class ObjectServiceImplTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
protected:
    ObjectStore::Asset asset_;
    std::string uri_;
    std::string appId_ = "ObjectServiceImplTest_appid_1";
    std::string sessionId_ = "123";
    std::vector<uint8_t> data_;
    std::string deviceId_ = "7001005458323933328a258f413b3900";
    uint64_t sequenceId_ = 10;
    uint64_t sequenceId_2 = 20;
    uint64_t sequenceId_3 = 30;
    std::string userId_ = "100";
    std::string bundleName_ = "test_bundleName";
    ObjectStore::AssetBindInfo assetBindInfo_;
    pid_t pid_ = 10;
    uint32_t tokenId_ = 100;
    static inline std::shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
};

void ObjectServiceImplTest::SetUp()
{
    uri_ = "file:://com.examples.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    ObjectStore::Asset asset{
        .id = "test_name",
        .name = uri_,
        .uri = uri_,
        .createTime = "2025.03.05",
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
        .path = "/data/storage/el2",
    };
    asset_ = asset;

    data_.push_back(sequenceId_);
    data_.push_back(sequenceId_2);
    data_.push_back(sequenceId_3);

    ObjectStore::AssetBindInfo AssetBindInfo{
        .storeName = "store_test",
        .tableName = "table_test",
        .field = "attachment",
        .assetName = "asset1.jpg",
    };
    assetBindInfo_ = AssetBindInfo;
    devMgrAdapterMock = make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = devMgrAdapterMock;
}

void ObjectServiceImplTest::TearDown()
{
    devMgrAdapterMock = nullptr;
}

/**
 * @tc.name: OnAssetChanged001
 * @tc.desc: OnAssetChanged test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, OnAssetChanged001, TestSize.Level1)
{
    std::string bundleName = "com.examples.hmos.notepad";
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(100, bundleName, 0);
        SetSelfTokenID(tokenId);

    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();

    // bundleName not equal tokenId
    auto ret = objectServiceImpl->OnAssetChanged(bundleName_, sessionId_, deviceId_, asset_);
    EXPECT_EQ(ret, OBJECT_PERMISSION_DENIED);
}

/**
 * @tc.name: BindAssetStore001
 * @tc.desc: BindAssetStore test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, BindAssetStore001, TestSize.Level1)
{
    std::string bundleName = "com.examples.hmos.notepad";
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(100, bundleName, 0);
        SetSelfTokenID(tokenId);

    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();

    // bundleName not equal tokenId
    auto ret = objectServiceImpl->BindAssetStore(bundleName_, sessionId_, asset_, assetBindInfo_);
    EXPECT_EQ(ret, OBJECT_PERMISSION_DENIED);
}

/**
 * @tc.name: DeleteSnapshot001
 * @tc.desc: DeleteSnapshot test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, DeleteSnapshot001, TestSize.Level1)
{
    std::string bundleName = "com.examples.hmos.notepad";
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(100, bundleName, 0);
        SetSelfTokenID(tokenId);

    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();

    // bundleName not equal tokenId
    auto ret = objectServiceImpl->DeleteSnapshot(bundleName_, sessionId_);
    EXPECT_EQ(ret, OBJECT_PERMISSION_DENIED);
}

/**
 * @tc.name: ResolveAutoLaunch001
 * @tc.desc: ResolveAutoLaunch test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, ResolveAutoLaunch001, TestSize.Level1)
{
    DistributedDB::AutoLaunchParam param {
        .userId = userId_,
        .appId = appId_,
        .storeId = "storeId",
    };
    std::string identifier = "identifier";
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    int32_t ret = objectServiceImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(ret, OBJECT_SUCCESS);
}

/**
 * @tc.name: OnInitialize001
 * @tc.desc: OnInitialize test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, OnInitialize001, TestSize.Level1)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    objectServiceImpl->executors_ = nullptr;
    DeviceInfo devInfo = { .uuid = "123" };
    EXPECT_CALL(*devMgrAdapterMock, GetLocalDevice())
        .WillOnce(Return(devInfo));
    int32_t ret = objectServiceImpl->OnInitialize();
    EXPECT_EQ(ret, OBJECT_INNER_ERROR);
}

/**
 * @tc.name: RegisterProgressObserver001
 * @tc.desc: RegisterProgressObserver test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, RegisterProgressObserver001, TestSize.Level1)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    sptr<IRemoteObject> callback;
    int32_t ret = objectServiceImpl->RegisterProgressObserver(bundleName_, sessionId_, callback);
    EXPECT_EQ(ret, OBJECT_PERMISSION_DENIED);
}

/**
 * @tc.name: UnregisterProgressObserver001
 * @tc.desc: UnregisterProgressObserver test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectServiceImplTest, UnregisterProgressObserver001, TestSize.Level1)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    int32_t ret = objectServiceImpl->UnregisterProgressObserver(bundleName_, sessionId_);
    EXPECT_EQ(ret, OBJECT_PERMISSION_DENIED);
}
}
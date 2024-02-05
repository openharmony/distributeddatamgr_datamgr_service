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

#include "cloud/asset_loader.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class AssetLoaderTest : public testing::Test {};

/**
* @tc.name: Download
* @tc.desc: download.
* @tc.type: FUNC
*/
HWTEST_F(AssetLoaderTest, Download, TestSize.Level1)
{
    AssetLoader loader;
    std::string tableName;
    std::string gid;
    Value prefix;
    VBucket assets;
    auto ret = loader.Download(tableName, gid, prefix, assets);
    ASSERT_EQ(ret, E_NOT_SUPPORT);
}

/**
* @tc.name: RemoveLocalAssets
* @tc.desc: remove local assets.
* @tc.type: FUNC
*/
HWTEST_F(AssetLoaderTest, RemoveLocalAssets, TestSize.Level1)
{
    AssetLoader loader;
    std::string tableName;
    std::string gid;
    Value prefix;
    VBucket assets;
    auto ret = loader.RemoveLocalAssets(tableName, gid, prefix, assets);
    ASSERT_EQ(ret, E_NOT_SUPPORT);
}

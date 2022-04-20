/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "directory_manager.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using namespace OHOS;

class DirectoryManagerTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() { }
    void TearDown() { }
};

/**
* @tc.name: GetStoragePath01
* @tc.desc: test get db dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: baoyayong
*/
HWTEST_F(DirectoryManagerTest, GetStoragePath01, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "10";
    metaData.bundleName = "com.sample.helloworld";
    metaData.dataDir = "/data/app/el1/10/com.sample.helloworld";
    metaData.securityLevel = SecurityLevel::S2;
    auto path = DirectoryManager::GetInstance().GetStorePath(metaData);
    EXPECT_EQ(path, metaData.dataDir);
}

/**
* @tc.name: GetStorageBackupPath01
* @tc.desc: test get db backup dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: baoyayong
*/
HWTEST_F(DirectoryManagerTest, GetStorageBackupPath01, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "10";
    metaData.bundleName = "com.sample.helloworld";
    metaData.dataDir = "/data/app/el1/10/com.sample.helloworld";
    metaData.securityLevel = SecurityLevel::S2;
    auto path = DirectoryManager::GetInstance().GetStoreBackupPath(metaData);
    EXPECT_EQ(path, metaData.dataDir + "/backup/");
}

/**
* @tc.name: GetStorageMetaPath01
* @tc.desc: test get db meta dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: baoyayong
*/
HWTEST_F(DirectoryManagerTest, GetStorageMetaPath01, TestSize.Level0)
{
    auto path = DirectoryManager::GetInstance().GetMetaDataStorePath();
    EXPECT_EQ(path, "/data/service/el1/public/distributeddata/meta/");
}

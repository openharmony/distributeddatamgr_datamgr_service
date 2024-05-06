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

#include "store/general_store.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class GeneralStoreTest : public testing::Test {};

/**
* @tc.name: MixMode
* @tc.desc: mix mode.
* @tc.type: FUNC
*/
HWTEST_F(GeneralStoreTest, MixMode, TestSize.Level1)
{
    uint32_t syncMode = GeneralStore::SyncMode::NEARBY_BEGIN;
    uint32_t highMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto ret = GeneralStore::MixMode(syncMode, highMode);
    ASSERT_EQ(ret, 0);
}

/**
* @tc.name: GetSyncMode
* @tc.desc: get sync mode.
* @tc.type: FUNC
*/
HWTEST_F(GeneralStoreTest, GetSyncMode, TestSize.Level1)
{
    uint32_t syncMode = GeneralStore::SyncMode::NEARBY_PULL;
    uint32_t highMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode = GeneralStore::MixMode(syncMode, highMode);
    auto ret = GeneralStore::GetSyncMode(mixMode);
    ASSERT_EQ(ret, syncMode);

    syncMode = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    ret = GeneralStore::GetSyncMode(mixMode);
    ASSERT_EQ(ret, syncMode);

    syncMode = GeneralStore::SyncMode::NEARBY_UNSUBSCRIBE_REMOTE;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    ret = GeneralStore::GetSyncMode(mixMode);
    ASSERT_EQ(ret, syncMode);
}

/**
* @tc.name: GetHighMode
* @tc.desc: get high mode.
* @tc.type: FUNC
*/
HWTEST_F(GeneralStoreTest, GetHighMode, TestSize.Level1)
{
    uint32_t syncMode = GeneralStore::SyncMode::NEARBY_PULL;
    uint32_t highMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode = GeneralStore::MixMode(syncMode, highMode);
    auto ret = GeneralStore::GetHighMode(mixMode);
    ASSERT_EQ(ret, highMode);
}

/**
* @tc.name: BindInfo
* @tc.desc: bind info.
* @tc.type: FUNC
*/
HWTEST_F(GeneralStoreTest, BindInfo, TestSize.Level1)
{
    std::shared_ptr<CloudDB> db;
    std::shared_ptr<AssetLoader> loader;
    GeneralStore::BindInfo bindInfo(db, loader);
    ASSERT_EQ(bindInfo.db_, db);
    ASSERT_EQ(bindInfo.loader_, loader);
}


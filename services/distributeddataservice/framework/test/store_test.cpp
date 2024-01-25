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
#define LOG_TAG "RefCountTest"

#include "accesstoken_kit.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "store/auto_cache.h"
#include "store/general_value.h"
#include "store/general_store.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class GenQueryTest : public testing::Test {
public:
};

class AutoCacheTest : public testing::Test {
public:
};

class GeneralStoreTest : public testing::Test {
public:
};

/**
* @tc.name: SetQueryNodestest
* @tc.desc:  Set QueryNodes.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
//HWTEST_F(GenQueryTest, GenQueryTest, TestSize.Level2)
//{
//    ZLOGI("GenQueryTest GenQueryTest begin.");
//    GenQuery *obj;
//    std::string tableName = "test_table";
//   // QueryNodes nodes = obj->GetQueryNodes(tableName);
//    //std::cout<<"nodes = "<< sizeof(nodes)<<std::endl;
//
//    //obj->SetQueryNodes(tableName,std::move(nodes));
//
//}

/**
* @tc.name: BindExecutorTest
* @tc.desc:  bind executor
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(AutoCacheTest, BindExecutorTest, TestSize.Level2)
{
    //�쳣���� metaDataΪ��
    ZLOGI("AutoCacheTest BindExecutorTest begin.");
    StoreMetaData metaData;
    AutoCache::Watchers watchers;
    auto store =  AutoCache::GetInstance().GetStore(metaData, watchers);
    ASSERT_EQ(store, nullptr);

    metaData.user = "0";
    metaData.bundleName = "ohos.test.demo";
    metaData.appId = "bundle_test_service";
    metaData.securityLevel = DistributedKv::SecurityLevel::S2;
    metaData.area = 1;
    metaData.tokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(10, "ohos.test.demo", 0);
    metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    store =  AutoCache::GetInstance().GetStore(metaData, watchers);
    //ASSERT_NE(store, nullptr);
}

/**
* @tc.name: GetMixModeTest
* @tc.desc: get mix mode
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(GeneralStoreTest, GetMixModeTest, TestSize.Level2)
{
    GeneralStore *genstore;
    uint32_t syncMode = 1;
    uint32_t highMode = 2;
    uint32_t mixMode =  genstore->MixMode(syncMode, highMode);
    ASSERT_EQ(mixMode, 3);

    uint32_t sync_Mode = genstore->GetSyncMode(mixMode);
    ASSERT_EQ(sync_Mode, 3);

    uint32_t high_Mode = genstore->GetHighMode(mixMode);
    ASSERT_EQ(high_Mode, 0);
}


}
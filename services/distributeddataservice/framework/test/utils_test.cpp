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
#define LOG_TAG "UtilsTest"
#include <chrono>

#include "access_token.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "utils/block_integer.h"
#include "utils/converter.h"
#include "utils/ref_count.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;

class UtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class BlockIntegerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class RefCountTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name: StoreMetaDataConvertToStoreInfo
 * @tc.desc: Storemeta data convert to storeinfo.
 * @tc.type: FUNC
 * @tc.require: AR000F8N0
 */
HWTEST_F(UtilsTest, StoreMetaDataConvertToStoreInfo, TestSize.Level2)
{
    ZLOGI("UtilsTest StoreMetaDataConvertToStoreInfo begin.");
    StoreMetaData metaData;
    metaData.bundleName = "ohos.test.demo1";
    metaData.storeId = "test_storeId";
    metaData.uid = OHOS::IPCSkeleton::GetCallingUid();
    metaData.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    CheckerManager::StoreInfo info = Converter::ConvertToStoreInfo(metaData);
    EXPECT_EQ(info.uid, metaData.uid);
    EXPECT_EQ(info.tokenId, metaData.tokenId);
    EXPECT_EQ(info.bundleName, metaData.bundleName);
    EXPECT_EQ(info.storeId, metaData.storeId);
}


/**
* @tc.name: SymbolOverloadingTest
* @tc.desc: Symbol Overloading
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(BlockIntegerTest, SymbolOverloadingTest, TestSize.Level2)
{
    ZLOGI("BlockIntegerTest SymbolOverloading begin.");
    int interval = 5;
    BlockInteger blockInteger(interval);

    blockInteger = 10;
    ASSERT_EQ(blockInteger, 10);

    auto now1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int val = blockInteger++;
    auto now2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    ASSERT_EQ(val, 10);
    ASSERT_EQ(blockInteger, 11);
    ASSERT_TRUE(now2 - now1 >= interval);

    now1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    val = ++blockInteger;
    now2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    ASSERT_EQ(val, 12);
    ASSERT_EQ(blockInteger, 12);
    ASSERT_TRUE(now2 - now1 >= interval);

    bool res = (blockInteger < 20);
    ASSERT_EQ(res, true);
}

/**
* @tc.name: ConstrucTortest
* @tc.desc:  Create Object.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RefCountTest, Constructortest, TestSize.Level2)
{
    ZLOGI("RefCountTest Constructor begin.");
    RefCount obj;
    ASSERT_NE(&obj, nullptr);

    int val = 0;
   {
        RefCount refCount([&val]() {
            ASSERT_EQ(val, 0);
            val = 10;
        });
        ASSERT_EQ(refCount, true);
   }
   ASSERT_EQ(val, 10);

   RefCount obj2(obj);
   ASSERT_EQ(obj2, obj);

   RefCount obj3 = obj;
   ASSERT_EQ(obj3, obj);
}

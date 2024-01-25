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
#define LOG_TAG "UtilsTest"
#include "utils/ref_count.h"

#include <type_traits>

#include "accesstoken_kit.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "metadata/store_meta_data.h"
#include "utils/block_integer.h"
#include "utils/converter.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class RefCountTest : public testing::Test {
public:
};

class BlockIntegerTest : public testing::Test {
public:
};

class ConverterTest : public testing::Test {
public:
};

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
    ASSERT_EQ(obj, false);

    RefCount obj1([]() { 
    });
    ASSERT_EQ(obj1, true);

    RefCount obj2(obj);
    ASSERT_EQ(obj2, obj);

    RefCount obj3 = obj;
    ASSERT_EQ(obj3, obj);

    RefCount obj4;
    obj4 = obj; 
    ASSERT_EQ(obj4, obj);

    bool isValid = (RefCount());
    ASSERT_EQ(isValid, false);
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
    BlockInteger blockInteger(1);
    blockInteger = 10;
    ASSERT_EQ(blockInteger, 10);

    blockInteger++;
    ASSERT_EQ(blockInteger, 11);

    int val = int(blockInteger);
    ASSERT_EQ(val, 11);

    ++blockInteger;
    ASSERT_EQ(blockInteger, 12);

    bool res = (blockInteger < 20);
    ASSERT_EQ(res,true);
}


/**
* @tc.name: StoreMetaDataConvertToStoreInfoTest
* @tc.desc: StoreMeta Data Convert To StoreInfo
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ConverterTest, StoreMetaDataConvertToStoreInfoTest, TestSize.Level2)
{
    ZLOGI("ConverterTest StoreMetaDataConvertToStoreInfoTest begin.");
    StoreMetaData metaData;
    metaData.user = "10";
    metaData.uid = 1;
    metaData.bundleName = "ohos.test.demo";
    metaData.storeId = "test_storeId";
    metaData.tokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(10, "ohos.test.demo", 0);
    CheckerManager::StoreInfo storeInfo  =  Converter::ConvertToStoreInfo(metaData);
    ASSERT_EQ(storeInfo.uid,1);
    ASSERT_EQ(storeInfo.tokenId,3);
    ASSERT_EQ(storeInfo.bundleName,"ohos.test.demo");
    ASSERT_EQ(storeInfo.storeId,"test_storeId");
}




} // namespace OHOS::Test
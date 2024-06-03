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
#define LOG_TAG "ServiceUtilsTest"
#include <chrono>

#include "access_token.h"
#include <endian.h>
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "utils/block_integer.h"
#include "utils/converter.h"
#include "utils/ref_count.h"
#include "utils/endian_converter.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class ServiceUtilsTest : public testing::Test {
public:
    static constexpr uint16_t HOST_VALUE16 = 0x1234;
    static constexpr uint16_t NET_VALUE16 = 0x3412;
    static constexpr uint32_t HOST_VALUE32 = 0x12345678;
    static constexpr uint32_t NET_VALUE32 = 0x78563412;
    static constexpr uint64_t HOST_VALUE64 = 0x1234567890ABCDEF;
    static constexpr uint64_t NET_VALUE64 = 0xEFCDAB8967452301;
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
    static uint64_t GetCurrentTime()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>
                (std::chrono::system_clock::now().time_since_epoch()).count());
    }
    int intervalTime = 100 * 1000;
    int testNum = 10;
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
 */
HWTEST_F(ServiceUtilsTest, StoreMetaDataConvertToStoreInfo, TestSize.Level2)
{
    ZLOGI("ServiceUtilsTest StoreMetaDataConvertToStoreInfo begin.");
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
    int interval = intervalTime;
    BlockInteger blockInteger(interval);
    blockInteger = testNum;
    ASSERT_EQ(blockInteger, testNum);

    auto now1 = GetCurrentTime();
    int val = blockInteger++;
    auto now2 = GetCurrentTime();
    ASSERT_EQ(val, 10);
    ASSERT_EQ(blockInteger, 11);
    ASSERT_TRUE(now2 - now1 >= interval);

    now1 = GetCurrentTime();
    val = ++blockInteger;
    now2 = GetCurrentTime();
    ASSERT_EQ(val, 12);
    ASSERT_EQ(blockInteger, 12);
    ASSERT_TRUE(now2 - now1 >= interval);
    ASSERT_TRUE(blockInteger < 20);
    int result = static_cast<int>(blockInteger);
    EXPECT_EQ(result, 12);
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
    int num = 0;
    {
        RefCount refCount([&num]() {
            num += 10;
        });
        ASSERT_TRUE(refCount);

        RefCount refCount1(refCount);
        ASSERT_TRUE(refCount1);

        RefCount refCount2(std::move(refCount));
        ASSERT_TRUE(refCount2);

        RefCount refCount3 = refCount1;
        ASSERT_TRUE(refCount3);

        RefCount refCount4 = std::move(refCount2);
        ASSERT_TRUE(refCount4);
        ASSERT_EQ(num, 0);
        EXPECT_TRUE(static_cast<bool>(refCount4));
    }
    ASSERT_EQ(num, 10);
}

/**
* @tc.name: HostToNet
* @tc.desc: test endian_converter HostToNet function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(ServiceUtilsTest, HostToNet, TestSize.Level1)
{
    uint16_t netValue16 = HostToNet(HOST_VALUE16);
    EXPECT_EQ(netValue16, htole16(HOST_VALUE16));

    uint16_t hostValue16 = NetToHost(NET_VALUE16);
    EXPECT_EQ(hostValue16, le16toh(NET_VALUE16));

    uint32_t netValue32 = HostToNet(HOST_VALUE32);
    EXPECT_EQ(netValue32, htole32(HOST_VALUE32));

    uint32_t hostValue32 = NetToHost(NET_VALUE32);
    EXPECT_EQ(hostValue32, le32toh(NET_VALUE32));

    uint64_t netValue64 = HostToNet(HOST_VALUE64);
    EXPECT_EQ(netValue64, htole64(HOST_VALUE64));

    uint64_t hostValue64 = NetToHost(NET_VALUE64);
    EXPECT_EQ(hostValue64, le64toh(NET_VALUE64));
}
} // namespace OHOS::Test
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
#define LOG_TAG "DataShareObsProxyTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "data_share_obs_proxy.h"
#include "datashare_errno.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
std::string BUNDLE_NAME = "ohos.datasharetest.demo";
constexpr int64_t TEST_SUB_ID = 100;
// length of int32 bytes
static constexpr int32_t INT32_BYTE_LEN = static_cast<int32_t>(sizeof(int32_t));

class DataShareObsProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

RdbChangeNode SampleRdbChangeNode()
{
    TemplateId tplId;
    tplId.subscriberId_ = TEST_SUB_ID;
    tplId.bundleName_ = BUNDLE_NAME;

    RdbChangeNode node;
    node.uri_ = std::string("");
    node.templateId_ = tplId;
    node.data_ = std::vector<std::string>();
    node.isSharedMemory_ = false;
    node.memory_ = nullptr;
    node.size_ = 0;

    return node;
}

/**
* @tc.name: CreateAshmem
* @tc.desc: test CreateAshmem function
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, CreateAshmem, TestSize.Level1)
{
    ZLOGI("CreateAshmem starts");
    RdbChangeNode node = SampleRdbChangeNode();
    
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);
    int ret = proxy.CreateAshmem(node);
    EXPECT_EQ(ret, DataShare::E_OK);
    EXPECT_NE(node.memory_, nullptr);
    ZLOGI("CreateAshmem ends");
}

/**
* @tc.name: WriteAshmem001
* @tc.desc: test WriteAshmem function. Write an int
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, WriteAshmem001, TestSize.Level1)
{
    ZLOGI("WriteAshmem001 starts");
    RdbChangeNode node = SampleRdbChangeNode();
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);
    int retCreate = proxy.CreateAshmem(node);
    EXPECT_EQ(retCreate, DataShare::E_OK);

    int len = 10;
    int32_t intLen = 4;
    int32_t offset = 0;
    int ret = proxy.WriteAshmem(node, (void *)&len, intLen, offset);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(offset, intLen);

    // read from the start
    const void *read = node.memory_->ReadFromAshmem(intLen, 0);
    EXPECT_NE(read, nullptr);
    int lenRead = *reinterpret_cast<const int *>(read);
    EXPECT_EQ(len, lenRead);
    ZLOGI("WriteAshmem001 ends");
}

/**
* @tc.name: WriteAshmem002
* @tc.desc: test WriteAshmem function. Write a str
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, WriteAshmem002, TestSize.Level1)
{
    ZLOGI("WriteAshmem002 starts");
    RdbChangeNode node = SampleRdbChangeNode();
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);
    int retCreate = proxy.CreateAshmem(node);
    EXPECT_EQ(retCreate, DataShare::E_OK);

    std::string string("Hello World");
    const char *str = string.c_str();
    int32_t len = string.length();
    int32_t offset = 0;
    int ret = proxy.WriteAshmem(node, (void *)str, len, offset);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(offset, len);

    // read from the start
    const void *read = node.memory_->ReadFromAshmem(len, 0);
    EXPECT_NE(read, nullptr);
    const char *strRead = reinterpret_cast<const char *>(read);
    std::string stringRead(strRead, len);
    EXPECT_EQ(stringRead, string);
    ZLOGI("WriteAshmem002 ends");
}

/**
* @tc.name: WriteAshmem003
* @tc.desc: test WriteAshmem function with error
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, WriteAshmem003, TestSize.Level1)
{
    ZLOGI("WriteAshmem003 starts");
    RdbChangeNode node = SampleRdbChangeNode();
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);
    
    OHOS::sptr<Ashmem> memory = Ashmem::CreateAshmem("WriteAshmem003", 2);
    EXPECT_NE(memory, nullptr);
    bool mapRet = memory->MapReadAndWriteAshmem();
    ASSERT_TRUE(mapRet);
    node.memory_ = memory;

    int len = 10;
    int32_t offset = 0;
    int ret = proxy.WriteAshmem(node, (void *)&len, 4, offset);
    EXPECT_EQ(ret, E_ERROR);
    ZLOGI("WriteAshmem003 ends");
}

/**
* @tc.name: SerializeDataIntoAshmem
* @tc.desc: test SerializeDataIntoAshmem function
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, SerializeDataIntoAshmem, TestSize.Level1)
{
    ZLOGI("SerializeDataIntoAshmem starts");
    RdbChangeNode node = SampleRdbChangeNode();

    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);

    int retCreate = proxy.CreateAshmem(node);
    EXPECT_EQ(retCreate, E_OK);

    // Push three times
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);

    // item length size + (str length size + str length) * 3
    int32_t offset = INT32_BYTE_LEN + (INT32_BYTE_LEN + static_cast<int32_t>(strlen(BUNDLE_NAME.c_str()))) * 3;
    int retSe = proxy.SerializeDataIntoAshmem(node);
    EXPECT_EQ(retSe, E_OK);
    EXPECT_EQ(node.size_, offset);

    offset = 0;
    const void *vecLenRead = node.memory_->ReadFromAshmem(INT32_BYTE_LEN, offset);
    EXPECT_NE(vecLenRead, nullptr);
    int vecLen = *reinterpret_cast<const int *>(vecLenRead);
    EXPECT_EQ(vecLen, 3);
    offset += INT32_BYTE_LEN;

    // 3 strings in the vec
    for (int i = 0; i < 3; i++) {
        const void *strLenRead = node.memory_->ReadFromAshmem(INT32_BYTE_LEN, offset);
        EXPECT_NE(strLenRead, nullptr);
        int32_t strLen = *reinterpret_cast<const int32_t *>(strLenRead);
        EXPECT_EQ(strLen, BUNDLE_NAME.length());
        offset += INT32_BYTE_LEN;

        const void *strRead = node.memory_->ReadFromAshmem(strLen, offset);
        EXPECT_NE(strRead, nullptr);
        const char *str = reinterpret_cast<const char *>(strRead);
        std::string stringRead(str, strLen);
        EXPECT_EQ(stringRead, BUNDLE_NAME);
        offset += strLen;
    }
    ZLOGI("SerializeDataIntoAshmem ends");
}

/**
* @tc.name: SerializeDataIntoAshmem002
* @tc.desc: test SerializeDataIntoAshmem function
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, SerializeDataIntoAshmem002, TestSize.Level1)
{
    ZLOGI("SerializeDataIntoAshmem starts");
    RdbChangeNode node = SampleRdbChangeNode();

    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);

    // Push three times
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);

    // memory too small for vec length
    OHOS::sptr<Ashmem> memory = Ashmem::CreateAshmem("SerializeDataIntoAshmem002", 2);
    EXPECT_NE(memory, nullptr);
    bool mapRet = memory->MapReadAndWriteAshmem();
    ASSERT_TRUE(mapRet);
    node.memory_ = memory;

    int retSe = proxy.SerializeDataIntoAshmem(node);
    EXPECT_EQ(retSe, E_ERROR);
    EXPECT_EQ(node.size_, 0);
    EXPECT_EQ(node.data_.size(), 3);
    ASSERT_FALSE(node.isSharedMemory_);

    // memory too small for string length
    OHOS::sptr<Ashmem> memory2 = Ashmem::CreateAshmem("SerializeDataIntoAshmem002", 6);
    EXPECT_NE(memory2, nullptr);
    mapRet = memory2->MapReadAndWriteAshmem();
    ASSERT_TRUE(mapRet);
    node.memory_ = memory2;

    retSe = proxy.SerializeDataIntoAshmem(node);
    EXPECT_EQ(retSe, E_ERROR);
    EXPECT_EQ(node.size_, 0);
    EXPECT_EQ(node.data_.size(), 3);
    ASSERT_FALSE(node.isSharedMemory_);

    // memory too small for string
    OHOS::sptr<Ashmem> memory3 = Ashmem::CreateAshmem("SerializeDataIntoAshmem002", 10);
    EXPECT_NE(memory3, nullptr);
    mapRet = memory3->MapReadAndWriteAshmem();
    ASSERT_TRUE(mapRet);
    node.memory_ = memory3;

    retSe = proxy.SerializeDataIntoAshmem(node);
    EXPECT_EQ(retSe, E_ERROR);
    EXPECT_EQ(node.size_, 0);
    EXPECT_EQ(node.data_.size(), 3);
    ASSERT_FALSE(node.isSharedMemory_);
   
    ZLOGI("SerializeDataIntoAshmem002 ends");
}

/**
* @tc.name: PreparationData
* @tc.desc: test PrepareRdbChangeNodeData function
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataShareObsProxyTest, PreparationData, TestSize.Level1)
{
    ZLOGI("PreparationData starts");
    RdbChangeNode node = SampleRdbChangeNode();
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);

    // Push three times, less than 200k
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);
    node.data_.push_back(BUNDLE_NAME);

    int ret = proxy.PrepareRdbChangeNodeData(node);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(node.data_.size(), 3);
    EXPECT_FALSE(node.isSharedMemory_);

    // Try to fake a 200k data. BUNDLE_NAME is 23 byte long and 7587 BUNDLE_NAMEs is over 200k.
    for (int i = 0; i < 7587; i++) {
        node.data_.push_back(BUNDLE_NAME);
    }
    ret = proxy.PrepareRdbChangeNodeData(node);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(node.data_.size(), 0);
    EXPECT_TRUE(node.isSharedMemory_);

    // Try to fake data over 10M. Write data of such size should fail because it exceeds the limit.
    for (int i = 0; i < 388362; i++) {
        node.data_.push_back(BUNDLE_NAME);
    }
    ret = proxy.PrepareRdbChangeNodeData(node);
    EXPECT_EQ(ret, E_ERROR);

    ZLOGI("PreparationData ends");
}
} // namespace OHOS::Test
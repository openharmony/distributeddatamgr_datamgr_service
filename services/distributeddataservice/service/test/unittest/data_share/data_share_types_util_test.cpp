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
#define LOG_TAG "DataShareTypesUtilTest"

#include <gtest/gtest.h>
#include <unistd.h>

#include "data_share_types_util.h"
#include "data_share_obs_proxy.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
std::string BUNDLE = "ohos.datasharetest.demo";
constexpr int64_t SUB_ID = 100;

class DataShareTypesUtilTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

RdbChangeNode CreateChangeNode()
{
    TemplateId tplId;
    tplId.subscriberId_ = SUB_ID;
    tplId.bundleName_ = BUNDLE;

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
* @tc.name: Unmarshalling_001
* @tc.desc: test Unmarshalling function and abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Unmarshalling_001, TestSize.Level1)
{
    ZLOGI("Unmarshalling_001 starts");
    Data data;
    data.version_ = 0;
    MessageParcel msgParcel;
    // msgParcel is empty, nothing to unmarshalling
    bool retVal = ITypesUtil::Unmarshalling(data, msgParcel);
    EXPECT_EQ(retVal, false);
    ZLOGI("Unmarshalling_001 ends");
}

/**
* @tc.name: Unmarshalling_002
* @tc.desc: test Unmarshalling function and abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Unmarshalling_002, TestSize.Level1)
{
    ZLOGI("Unmarshalling_002 starts");
    AshmemNode node;
    MessageParcel msgParcel;
    bool retVal = ITypesUtil::Unmarshalling(node, msgParcel);
    EXPECT_EQ(retVal, true);
    ZLOGI("Unmarshalling_002 ends");
}

/**
* @tc.name: Unmarshalling_003
* @tc.desc: test Unmarshalling function abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Unmarshalling_003, TestSize.Level1)
{
    ZLOGI("Unmarshalling_003 starts");
    ITypesUtil::Predicates predicates;
    MessageParcel msgParcel;
    bool retVal = ITypesUtil::Unmarshalling(predicates, msgParcel);
    EXPECT_EQ(retVal, false);
    ZLOGI("Unmarshalling_003 ends");
}

/**
* @tc.name: Marshalling_001
* @tc.desc: test Marshalling function abnormal scene
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Marshalling_001, TestSize.Level1)
{
    ZLOGI("Marshalling_001 starts");
    RdbChangeNode node = CreateChangeNode();
    node.isSharedMemory_ = true;
    node.memory_ = nullptr;
    MessageParcel msgParcel;
    bool retVal = ITypesUtil::Marshalling(node, msgParcel);
    EXPECT_EQ(retVal, false);
    ZLOGI("Marshalling_001 ends");
}

/**
* @tc.name: Marshalling_002
* @tc.desc: test Marshalling function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Marshalling_002, TestSize.Level1)
{
    ZLOGI("Marshalling_002 starts");
    RdbChangeNode node = CreateChangeNode();
    node.isSharedMemory_ = false;
    node.memory_ = nullptr;
    MessageParcel msgParcel;
    bool retVal = ITypesUtil::Marshalling(node, msgParcel);
    EXPECT_EQ(retVal, true);
    ZLOGI("Marshalling_002 ends");
}

/**
* @tc.name: Marshalling_003
* @tc.desc: test Marshalling function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareTypesUtilTest, Marshalling_003, TestSize.Level1)
{
    ZLOGI("Marshalling_003 starts");
    RdbChangeNode node = CreateChangeNode();
    OHOS::sptr<IRemoteObject> fake = nullptr;
    RdbObserverProxy proxy(fake);
    int retCreate = proxy.CreateAshmem(node);
    EXPECT_EQ(retCreate, DataShare::E_OK);

    int len = 10;
    int intLen = 4;
    int offset = 0;
    int ret = proxy.WriteAshmem(node, (void *)&len, intLen, offset);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(offset, intLen);

    // read from the start
    const void *read = node.memory_->ReadFromAshmem(intLen, 0);
    EXPECT_NE(read, nullptr);

    node.isSharedMemory_ = true;

    MessageParcel msgParcel;
    bool retVal = ITypesUtil::Marshalling(node, msgParcel);
    EXPECT_EQ(retVal, true);
    ZLOGI("Marshalling_003 ends");
}
}
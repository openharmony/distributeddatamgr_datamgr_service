/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "iremote_object.h"
#include "itypes_util.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
class TypesUtilTest : public testing::Test {
public:
    class TestRemoteObject : public IRemoteObject {
    public:
        int32_t GetObjectRefCount() override
        {
            return 0;
        }
        int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
        {
            return 0;
        }
        bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
        {
            return false;
        }
        bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
        {
            return false;
        }
        int Dump(int fd, const vector<std::u16string> &args) override
        {
            return 0;
        }
    };
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

HWTEST_F(TypesUtilTest, DeviceInfo, TestSize.Level0)
{
    MessageParcel parcel;
    DeviceInfo clientDev;
    clientDev.deviceId = "123";
    clientDev.deviceName = "rk3568";
    clientDev.deviceType = "phone";
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, clientDev));
    DeviceInfo serverDev;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, serverDev));
    ASSERT_EQ(clientDev.deviceId, serverDev.deviceId);
    ASSERT_EQ(clientDev.deviceName, serverDev.deviceName);
    ASSERT_EQ(clientDev.deviceType, serverDev.deviceType);
}

HWTEST_F(TypesUtilTest, Entry, TestSize.Level0)
{
    MessageParcel parcel;
    Entry entryIn;
    entryIn.key = "student_name_mali";
    entryIn.value = "age:20";
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, entryIn));
    Entry entryOut;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, entryOut));
    EXPECT_EQ(entryOut.key.ToString(), std::string("student_name_mali"));
    EXPECT_EQ(entryOut.value.ToString(), std::string("age:20"));
}

HWTEST_F(TypesUtilTest, ChangeNotification, TestSize.Level1)
{
    Entry insert, update, del;
    insert.key = "insert";
    update.key = "update";
    del.key = "delete";
    insert.value = "insert_value";
    update.value = "update_value";
    del.value = "delete_value";
    std::vector<Entry> inserts, updates, deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), std::string(), false);
    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, changeIn));
    ChangeNotification changeOut({}, {}, {}, "", false);
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, changeOut));
    ASSERT_EQ(changeOut.GetInsertEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetInsertEntries().front().key.ToString(), std::string("insert"));
    EXPECT_EQ(changeOut.GetInsertEntries().front().value.ToString(), std::string("insert_value"));
    ASSERT_EQ(changeOut.GetUpdateEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetUpdateEntries().front().key.ToString(), std::string("update"));
    EXPECT_EQ(changeOut.GetUpdateEntries().front().value.ToString(), std::string("update_value"));
    ASSERT_EQ(changeOut.GetDeleteEntries().size(), 1UL);
    EXPECT_EQ(changeOut.GetDeleteEntries().front().key.ToString(), std::string("delete"));
    EXPECT_EQ(changeOut.GetDeleteEntries().front().value.ToString(), std::string("delete_value"));
    EXPECT_EQ(changeOut.IsClear(), false);
}


HWTEST_F(TypesUtilTest, Multiple, TestSize.Level1)
{
    uint32_t input1 = 10;
    int32_t input2 = -10;
    std::string input3 = "i test";
    Blob input4 = "input 4";
    Entry input5;
    input5.key = "my test";
    input5.value = "test value";
    DeviceInfo input6 = {.deviceId = "mock deviceId", .deviceName = "mock phone", .deviceType = "0"};
    sptr<IRemoteObject> input7 = new TestRemoteObject();
    MessageParcel parcel;
    ASSERT_TRUE(ITypesUtil::Marshal(parcel, input1, input2, input3, input4, input5, input6, input7));
    uint32_t output1 = 0;
    int32_t output2 = 0;
    std::string output3 = "";
    Blob output4;
    Entry output5;
    DeviceInfo output6;
    sptr<IRemoteObject> output7;
    ASSERT_TRUE(ITypesUtil::Unmarshal(parcel, output1, output2, output3, output4, output5, output6, output7));
    ASSERT_EQ(output1, input1);
    ASSERT_EQ(output2, input2);
    ASSERT_EQ(output3, input3);
    ASSERT_EQ(output4, input4);
    ASSERT_EQ(output5.key, input5.key);
    ASSERT_EQ(output5.value, input5.value);
    ASSERT_EQ(output6.deviceId, input6.deviceId);
    ASSERT_EQ(output6.deviceName, input6.deviceName);
    ASSERT_EQ(output6.deviceType, input6.deviceType);
    ASSERT_EQ(output7, input7);
}
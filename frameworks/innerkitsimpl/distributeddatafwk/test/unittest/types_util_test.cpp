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
#include "types.h"
#include "itypes_util.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
class TypesUtilTest : public testing::Test {
public:
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
    ASSERT_TRUE(ITypesUtil::Marshalling(clientDev, parcel));
    DeviceInfo serverDev;
    ASSERT_TRUE(ITypesUtil::Unmarshalling(parcel, serverDev));
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
    ASSERT_TRUE(ITypesUtil::Marshalling(entryIn, parcel));
    Entry entryOut;
    ASSERT_TRUE(ITypesUtil::Unmarshalling(parcel, entryOut));
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
    ASSERT_TRUE(ITypesUtil::Marshalling(changeIn, parcel));
    ChangeNotification changeOut({}, {}, {}, "", false);
    ASSERT_TRUE(ITypesUtil::Unmarshalling(parcel, changeOut));
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
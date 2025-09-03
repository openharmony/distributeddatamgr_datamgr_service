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
#define LOG_TAG "SerializableTest"
#include <type_traits>
#include "log_print.h"
#include "serializable/serializable.h"
#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class SerializableTest : public testing::Test {
public:
    struct Normal final : public Serializable {
    public:
        std::string name = "Test";
        int32_t count = 0;
        uint32_t status = 1;
        int64_t value = 2;
        bool isClear = false;
        std::vector<std::string> cols{ "123", "345", "789" };
        std::vector<std::vector<int32_t>> colRow{ { 123, 345, 789 }, { 123, 345, 789 } };

        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(name)], name);
            SetValue(node[GET_NAME(count)], count);
            SetValue(node[GET_NAME(status)], status);
            SetValue(node[GET_NAME(value)], value);
            SetValue(node[GET_NAME(isClear)], isClear);
            SetValue(node[GET_NAME(cols)], cols);
            SetValue(node[GET_NAME(colRow)], colRow);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(name), name);
            GetValue(node, GET_NAME(count), count);
            GetValue(node, GET_NAME(status), status);
            GetValue(node, GET_NAME(value), value);
            GetValue(node, GET_NAME(isClear), isClear);
            GetValue(node, GET_NAME(cols), cols);
            GetValue(node, GET_NAME(colRow), colRow);
            return true;
        }
        bool operator == (const Normal &ref) const
        {
            return name == ref.name && count == ref.count && status == ref.status && value == ref.value
                && isClear == ref.isClear && cols == ref.cols;
        }
    };

    struct NormalEx final : public Serializable {
    public:
        std::vector<Normal> normals {Normal(), Normal()};
        Normal normal;
        int32_t count = 123;
        std::string name = "wdt";
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(normals)], normals);
            SetValue(node[GET_NAME(normal)], normal);
            SetValue(node[GET_NAME(count)], count);
            SetValue(node[GET_NAME(name)], name);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(normals), normals);
            GetValue(node, GET_NAME(normal), normal);
            GetValue(node, GET_NAME(count), count);
            GetValue(node, GET_NAME(name), name);
            return true;
        }
        bool operator==(const NormalEx &normalEx) const
        {
            return normals == normalEx.normals && count == normalEx.count && name == normalEx.name;
        }
    };
    static void SetUpTestCase(void)
    {
    }
    static void TearDownTestCase(void)
    {
    }
    void SetUp()
    {
        Test::SetUp();
    }
    void TearDown()
    {
        Test::TearDown();
    }

    template<typename T>
    static inline bool EqualPtr(const T *src, const T *target)
    {
        return (((src) == (target)) || ((src) != nullptr && (target) != nullptr && *(src) == *(target)));
    }
};

/**
* @tc.name: SerializableSuiteGetVal
* @tc.desc: Get Value.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, GetNormalVal, TestSize.Level2)
{
    ZLOGI("SerializableSuite GetNormalVal begin.");
    Normal normal;
    normal.name = "normal";
    normal.count = -1;
    normal.status = 12;
    normal.value = -56;
    normal.isClear = true;
    normal.cols = {"adfasdfas"};
    auto json = normal.Marshall();
    auto jstr = to_string(normal.Marshall());
    Normal normal1;
    normal1.Unmarshall(jstr);
    ASSERT_TRUE(normal == normal1) << normal1.name;
    ASSERT_FALSE(normal1.Unmarshall(""));
    ASSERT_FALSE(normal1.Unmarshall("{"));
    std::vector<std::string> testVec = {"adfasdfas", "banana"};
    ASSERT_FALSE(json["cols"] == testVec);
}

/**
* @tc.name: Delete Serializable
* @tc.desc: can delete child class, but not delete parent class point.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, DeleteSerializable, TestSize.Level2)
{
    ZLOGI("SerializableSuite DeleteSerializable begin.");
    ASSERT_FALSE(std::is_destructible<Serializable>::value);
    ASSERT_TRUE(std::is_destructible<NormalEx>::value);
}

/**
* @tc.name: SerializableSuiteGetMutilVal
* @tc.desc: mutil value case.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, GetMutilVal, TestSize.Level2)
{
    ZLOGI("SerializableSuite GetMutilVal begin.");
    NormalEx normalEx;
    normalEx.normals = {Normal()};
    normalEx.name = "normalEx";
    auto jstr = to_string(normalEx.Marshall());
    NormalEx normal1;
    normal1.Unmarshall(jstr);
    ASSERT_TRUE(normalEx == normal1) << normal1.name;
}

/**
* @tc.name: GetMap
* @tc.desc: mutil value case.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, GetMap, TestSize.Level2)
{
    ZLOGI("SerializableSuite GetMapVals begin.");
    std::map<std::string, NormalEx> marshData;
    NormalEx normalEx;
    normalEx.normals = { Normal() };
    normalEx.name = "normalEx";
    marshData.insert(std::pair{ "test1", normalEx });
    auto jsonData = NormalEx::Marshall(marshData);

    std::map<std::string, NormalEx> unmarshData;
    NormalEx::Unmarshall(jsonData, unmarshData);
    ASSERT_TRUE((marshData["test1"] == unmarshData["test1"])) << jsonData;
}

/**
* @tc.name: GetMapInStruct
* @tc.desc: mutil value case.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, GetMapInStruct, TestSize.Level2)
{
    struct TestMeta : public Serializable {
        std::map<std::string, NormalEx> data;
        std::map<std::string, bool> *index = nullptr;
        std::vector<std::map<std::string, NormalEx>> others;
        ~TestMeta()
        {
            delete index;
            index = nullptr;
        }
        bool Marshal(json &node) const
        {
            SetValue(node[GET_NAME(data)], data);
            SetValue(node[GET_NAME(index)], index);
            SetValue(node[GET_NAME(others)], others);
            return true;
        }

        bool Unmarshal(const json &node)
        {
            GetValue(node, GET_NAME(data), data);
            GetValue(node, GET_NAME(index), index);
            GetValue(node, GET_NAME(others), others);
            return true;
        }
    };
    ZLOGI("SerializableSuite GetMapVals begin.");
    TestMeta marData;
    NormalEx normalEx;
    normalEx.normals = { Normal() };
    normalEx.name = "normalEx";
    marData.data.insert(std::pair{ "test1", normalEx });
    marData.others.push_back({ std::pair{ "test2", normalEx } });
    marData.index = new (std::nothrow) std::map<std::string, bool>;
    ASSERT_NE(marData.index, nullptr);
    marData.index->insert(std::pair{ "test1", true });
    marData.index->insert(std::pair{ "test2", true });
    auto jsonData = NormalEx::Marshall(marData);
    TestMeta unmarData;
    NormalEx::Unmarshall(jsonData, unmarData);
    ASSERT_TRUE((marData.data == unmarData.data)) << jsonData;
    ASSERT_TRUE((marData.others == unmarData.others)) << jsonData;
    ASSERT_NE(unmarData.index, nullptr);
    ASSERT_TRUE((*marData.index == *unmarData.index)) << jsonData;
}

/**
* @tc.name: GetTestValue
* @tc.desc: set value with point param.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(SerializableTest, SetPointerValue, TestSize.Level2)
{
    struct Test final : public Serializable {
    public:
        int32_t *count = nullptr;
        int64_t *value = nullptr;
        uint32_t *status = nullptr;
        bool *isClear = nullptr;
        ~Test()
        {
            delete count;
            count = nullptr;
            delete value;
            value = nullptr;
            delete status;
            status = nullptr;
            delete isClear;
            isClear = nullptr;
        }
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(count)], count);
            SetValue(node[GET_NAME(status)], status);
            SetValue(node[GET_NAME(value)], value);
            SetValue(node[GET_NAME(isClear)], isClear);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(count), count);
            GetValue(node, GET_NAME(status), status);
            GetValue(node, GET_NAME(value), value);
            GetValue(node, GET_NAME(isClear), isClear);
            return true;
        }
        bool operator==(const Test &test) const
        {
            return (EqualPtr(count, test.count)) && (EqualPtr(status, test.status)) &&
                (EqualPtr(value, test.value)) && (EqualPtr(isClear, test.isClear));
        }
    };
    Test in;
    in.count = new int32_t(100);
    in.value = new int64_t(-100);
    in.status = new uint32_t(110);
    in.isClear = new bool(true);
    auto json = to_string(in.Marshall());
    Test out;
    out.Unmarshall(json);
    ASSERT_TRUE(in == out) << in.count;
}

/**
* @tc.name: IsJson
* @tc.desc: is json.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, IsJson, TestSize.Level1)
{
    std::string str = "test";
    std::string jsonStr = "\"test\"";
    ASSERT_FALSE(Serializable::IsJson(str));
    ASSERT_TRUE(Serializable::IsJson(jsonStr));
}

/**
* @tc.name: ToJson_01
* @tc.desc: to json.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, ToJson_01, TestSize.Level1)
{
    std::string jsonStr = "{\"key\":\"value\"}";
    Serializable::json result = Serializable::ToJson(jsonStr);
    ASSERT_FALSE(result.is_discarded());
}

/**
* @tc.name: ToJson_02
* @tc.desc: to json.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, ToJson_02, TestSize.Level1)
{
    std::string jsonStr = "invalid_json";
    Serializable::json result = Serializable::ToJson(jsonStr);
    ASSERT_FALSE(result.is_discarded());
}

/**
* @tc.name: ToJson_03
* @tc.desc: to json.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, ToJson_03, TestSize.Level1)
{
    std::string jsonStr = "";
    Serializable::json result = Serializable::ToJson(jsonStr);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: ToJson_04
* @tc.desc: to json.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, ToJson_04, TestSize.Level1)
{
    std::string jsonStr = "{invalid_json}";
    Serializable::json result = Serializable::ToJson(jsonStr);
    ASSERT_FALSE(result.is_discarded());
}

/**
* @tc.name: ToJson_05
* @tc.desc: test string to json of value with numeric type.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, ToJson_05, TestSize.Level1)
{
    std::string jsonStr = "{\"key\": 10}";
    Serializable::json result = Serializable::ToJson(jsonStr);
    uint64_t uint64Value;
    bool ret = Serializable::GetValue(result, "key", uint64Value);
    ASSERT_TRUE(ret);

    std::string jsonStr2 = "{\"key\": 10.0}";
    Serializable::json result2 = Serializable::ToJson(jsonStr2);
    double doubleValue;
    ret = Serializable::GetValue(result2, "key", doubleValue);
    ASSERT_TRUE(ret);
}

/**
* @tc.name: GetValueTest001
* @tc.desc: Test to json when type not match.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, GetValueTest001, TestSize.Level1)
{
    std::string jsonStr = "{\"key\": 10}";
    Serializable::json result = Serializable::ToJson(jsonStr);

    std::string value;
    bool ret = Serializable::GetValue(result, "key", value);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(result, "notExist", value);
    ASSERT_FALSE(ret);

    std::string jsonStr2 = "{\"key\": \"str\"}";
    Serializable::json strResult = Serializable::ToJson(jsonStr2);
    int32_t intValue;
    ret = Serializable::GetValue(strResult, "key", intValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", intValue);
    ASSERT_FALSE(ret);

    uint32_t uintValue;
    ret = Serializable::GetValue(strResult, "key", uintValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", uintValue);
    ASSERT_FALSE(ret);

    uint64_t uint64Value;
    ret = Serializable::GetValue(strResult, "key", uint64Value);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", uint64Value);
    ASSERT_FALSE(ret);

    int64_t int64Value;
    ret = Serializable::GetValue(strResult, "key", int64Value);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", int64Value);
    ASSERT_FALSE(ret);

    bool boolValue;
    ret = Serializable::GetValue(strResult, "key", boolValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", boolValue);
    ASSERT_FALSE(ret);
    
    double doubleValue;
    ret = Serializable::GetValue(strResult, "key", doubleValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", doubleValue);
    ASSERT_FALSE(ret);

    std::vector<uint8_t> arrayValue;
    ret = Serializable::GetValue(strResult, "key", arrayValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", arrayValue);
    ASSERT_FALSE(ret);
}

/**
* @tc.name: SetUintValue
* @tc.desc: set value with uint param.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, SetUintValue, TestSize.Level2)
{
    struct TestUint final : public Serializable {
    public:
        std::vector<uint8_t> testBytes = { 0x01, 0x02, 0x03, 0x04 };
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(testBytes)], testBytes);
            return true;
        }
 
        bool Unmarshal(const json &node) override
        {
            bool success = true;
            success = GetValue(node, GET_NAME(testBytes), testBytes) && success;
            return success;
        }
 
        bool operator==(const TestUint &other) const
        {
            return testBytes == other.testBytes;
        }
    };

    std::string jsonStr2 = "{\"key\": \"str\"}";
    Serializable::json strResult = Serializable::ToJson(jsonStr2);
    TestUint serialValue;
    bool ret = Serializable::GetValue(strResult, "key", serialValue);
    ASSERT_FALSE(ret);
    ret = Serializable::GetValue(strResult, "notExist", serialValue);
    ASSERT_FALSE(ret);
}

/**
* @tc.name: SetStringMapValue
* @tc.desc: set map value with string param.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, SetStringMapValue, TestSize.Level2)
{
    struct TestStringMap final : public Serializable {
    public:
        std::map<std::string, std::string> testMap = {
            {"name", "John"},
            {"email", "john@example.com"}
        };
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(testMap)], testMap);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(testMap), testMap);
            return true;
        }
        bool operator==(const TestStringMap &other) const
        {
            return testMap == other.testMap;
        }
    };
 
    TestStringMap in;
    in.testMap["name"] = "New York";
    in.testMap["email"] = "john@sample.com";
    auto json = to_string(in.Marshall());
    TestStringMap out;
    out.Unmarshall(json);
    ASSERT_TRUE(in == out);
}
 
/**
* @tc.name: SetStringMapValue
* @tc.desc: set map value with int param.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, SetMapValue, TestSize.Level2)
{
    struct TestMap final : public Serializable {
    public:
        std::map<std::string, uint64_t> testMap = {
            {"id", 123456},
            {"version", 42}
        };
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(testMap)], testMap);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(testMap), testMap);
            return true;
        }
        bool operator==(const TestMap &other) const
        {
            return testMap == other.testMap;
        }
    };
 
    TestMap in;
    in.testMap["version"] = 552;
    auto json = to_string(in.Marshall());
    TestMap out;
    out.Unmarshall(json);
    ASSERT_TRUE(in == out);
}
 
/**
* @tc.name: BoundaryTest
* @tc.desc: test boundary.
* @tc.type: FUNC
*/
HWTEST_F(SerializableTest, BoundaryTest, TestSize.Level1)
{
    struct TestBoundary : public Serializable {
        int32_t int32Val;
        uint32_t uint32Val;
        int64_t int64Val;
        uint64_t uint64Val;
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(int32Val)], int32Val);
            SetValue(node[GET_NAME(uint32Val)], uint32Val);
            SetValue(node[GET_NAME(int64Val)], int64Val);
            SetValue(node[GET_NAME(uint64Val)], uint64Val);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            bool success = true;
            success = GetValue(node, GET_NAME(int32Val), int32Val) && success;
            success = GetValue(node, GET_NAME(uint32Val), uint32Val) && success;
            success = GetValue(node, GET_NAME(int64Val), int64Val) && success;
            success = GetValue(node, GET_NAME(uint64Val), uint64Val) && success;
            return success;
        }
    };
    TestBoundary in, out;
    in.int32Val = INT32_MIN;
    in.uint32Val = 0;
    in.int64Val = INT64_MIN;
    in.uint64Val = 0;
 
    auto json = to_string(in.Marshall());
    out.Unmarshall(json);
    EXPECT_EQ(out.int32Val, in.int32Val);
    EXPECT_EQ(out.uint32Val, in.uint32Val);
    EXPECT_EQ(out.int64Val, in.int64Val);
    EXPECT_EQ(out.uint64Val, in.uint64Val);
 
    in.int32Val = INT32_MAX;
    in.uint32Val = UINT32_MAX;
    in.int64Val = INT64_MAX;
    in.uint64Val = UINT64_MAX;
 
    json = to_string(in.Marshall());
    out.Unmarshall(json);
    EXPECT_EQ(out.int32Val, in.int32Val);
    EXPECT_EQ(out.uint32Val, in.uint32Val);
    EXPECT_EQ(out.int64Val, in.int64Val);
    EXPECT_EQ(out.uint64Val, in.uint64Val);
}
} // namespace OHOS::Test
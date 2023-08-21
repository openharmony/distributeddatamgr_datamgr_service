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

    protected:
        ~Normal() = default;
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
    auto jstr = to_string(normal.Marshall());
    Normal normal1;
    normal1.Unmarshall(jstr);
    ASSERT_TRUE(normal == normal1) << normal1.name;
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
HWTEST_F(SerializableTest, GetTestValue, TestSize.Level2)
{
    struct Test final : public Normal {
    public:
        bool Marshal(json &node) const override
        {
            SetValue(node[GET_NAME(name)], &name);
            SetValue(node[GET_NAME(count)], &count);
            SetValue(node[GET_NAME(status)], &status);
            SetValue(node[GET_NAME(value)], &value);
            SetValue(node[GET_NAME(isClear)], &isClear);
            return true;
        }
        bool Unmarshal(const json &node) override
        {
            GetValue(node, GET_NAME(name), name);
            GetValue(node, GET_NAME(count), count);
            GetValue(node, GET_NAME(status), status);
            GetValue(node, GET_NAME(value), value);
            GetValue(node, GET_NAME(isClear), isClear);
            return true;
        }
    };
    Test in;
    in.name = "input";
    in.count = 10000;
    in.status = 0;
    in.value = -3;
    in.isClear = true;
    auto json = to_string(in.Marshall());
    Test out;
    out.Unmarshall(json);
    ASSERT_TRUE(in == out) << in.name;
}
} // namespace OHOS::Test
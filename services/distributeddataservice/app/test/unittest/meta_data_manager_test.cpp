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
#define LOG_TAG "MetaDataManagerTest"

#include "metadata/meta_data_manager.h"

#include "gtest/gtest.h"
#include "kvstore_meta_manager.h"
#include "log_print.h"
#include "semaphore_ex.h"

namespace {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using KvStoreMetaManager = OHOS::DistributedKv::KvStoreMetaManager;
constexpr const char *TEST_KEY = "Hop";
class MetaDataManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        KvStoreMetaManager::GetInstance().InitMetaParameter();
        KvStoreMetaManager::GetInstance().InitMetaListener();
    }
    static void TearDownTestCase()
    {
    }
    void SetUp()
    {
        DeleteTestData();
    }
    void TearDown()
    {
        DeleteTestData();
    }

private:
    void DeleteTestData()
    {
        std::string testKey(TEST_KEY);
        MetaDataManager::GetInstance().DelMeta(testKey);
    }
};

class Student final : public Serializable {
public:
    std::string name;
    int32_t age;

    bool Marshal(json &node) const
    {
        bool ret = true;
        ret = SetValue(node[GET_NAME(name)], name) && ret;
        ret = SetValue(node[GET_NAME(age)], age) && ret;
        return ret;
    }

    bool Unmarshal(const json &node)
    {
        bool ret = true;
        ret = GetValue(node, GET_NAME(name), name) && ret;
        ret = GetValue(node, GET_NAME(age), age) && ret;
        return ret;
    }
};

/**
* @tc.name: SaveMeta
* @tc.desc: test save meta
* @tc.type: FUNC
* @tc.require:
* @tc.author: illybyy
*/
HWTEST_F(MetaDataManagerTest, MetaBasic_01, TestSize.Level1)
{
    ZLOGI("begin");
    Student student;
    student.name = TEST_KEY;
    student.age = 21;

    OHOS::Semaphore sem(0);
    std::string changedKey;
    auto prefix = student.name.substr(0, 1);
    MetaDataManager::GetInstance().Subscribe(
        prefix, [&changedKey, &sem](const std::string &key, const std::string &value, int32_t action) {
            changedKey = key;
            sem.Post();
            return true;
        });

    auto result = MetaDataManager::GetInstance().SaveMeta(student.name, student);
    ASSERT_TRUE(result);
    sem.Wait();
    EXPECT_TRUE(student.name == changedKey);
    MetaDataManager::GetInstance().Unsubscribe(prefix);

    Student student1;
    result = MetaDataManager::GetInstance().LoadMeta(student.name, student1);
    ASSERT_TRUE(result);
    EXPECT_TRUE(student.name == student1.name);
    EXPECT_TRUE(student.age == student1.age);

    ZLOGI("end");
}
} // namespace
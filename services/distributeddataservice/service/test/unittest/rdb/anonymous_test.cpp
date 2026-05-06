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
#define LOG_TAG "AnonymousTest"

#include "gtest/gtest.h"
#include "log_print.h"
#include "utils/anonymous.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;

namespace OHOS::Test {
namespace DistributedRDBTest {
class AnonymousTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: AnonymousTest001
* @tc.desc: anonymous with path and fileName test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(AnonymousTest, AnonymousTest001, TestSize.Level1)
{
    std::string path = "/abc/def/ghijkl.txt";
    std::string anonymousName = Anonymous::Change(path);
    EXPECT_EQ(anonymousName, "/***/ghi***txt");
    path = "/abc/el/ghijkl.txt";
    anonymousName = Anonymous::Change(path);
    EXPECT_EQ(anonymousName, "/***/ghi***txt");
    path = "/a/elsuffix/ghijkl.txt";
    anonymousName = Anonymous::Change(path);
    EXPECT_EQ(anonymousName, "/***/els/***/ghi***txt");
    path = "/abc/el/a.txt";
    anonymousName = Anonymous::Change(path);
    EXPECT_EQ(anonymousName, "/***/a.t***");
    path = "/abc/el/file.txt";
    anonymousName = Anonymous::Change(path);
    EXPECT_EQ(anonymousName, "/***/fil***");
    std::string fileName = "";
}
}
}
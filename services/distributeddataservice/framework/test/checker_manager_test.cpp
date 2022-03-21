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

#include "checker/checker_manager.h"
#include "utils/crypto.h"
#include <gtest/gtest.h>
using namespace testing::ext;
using namespace OHOS::DistributedData;
class CheckerManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};
/**
* @tc.name: checkers
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, Checkers, TestSize.Level0)
{
    auto *checker = CheckerManager::GetInstance().GetChecker("SystemChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("BundleChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("OtherChecker");
    ASSERT_EQ(checker, nullptr);
}

/**
* @tc.name: SystemChecker bms
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerBMS, TestSize.Level0)
{
    ASSERT_EQ("bundle_manager_service", CheckerManager::GetInstance().GetAppId("bundle_manager_service", 1000));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid("bundle_manager_service", 1000));
}

/**
* @tc.name: SystemChecker form
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerForm, TestSize.Level0)
{
    ASSERT_EQ("form_storage", CheckerManager::GetInstance().GetAppId("form_storage", 1000));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid("form_storage", 1000));
}

/**
* @tc.name: SystemChecker ivi
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerIVI, TestSize.Level0)
{
    ASSERT_EQ("ivi_config_manager", CheckerManager::GetInstance().GetAppId("ivi_config_manager", 1000));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid("ivi_config_manager", 1000));
}

/**
* @tc.name: BundleChecker
* @tc.desc: checker the bundle name of the bundle abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, BundleChecker, TestSize.Level0)
{
    ASSERT_EQ(Crypto::Sha256("ohos.test.demo"),
        CheckerManager::GetInstance().GetAppId("ohos.test.demo", 100000));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid("ohos.test.demo", 100000));
}

/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "backuprule/backup_rule_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class BackupRuleManagerTest : public testing::Test {
public:
    class TestRule : public BackupRuleManager::BackupRule {
    public:
        TestRule()
        {
            BackupRuleManager::GetInstance().RegisterPlugin("TestRule", [this]() -> auto {
                return this;
            });
        }
        bool CanBackup() override
        {
            return false;
        }
    };
};

/**
* @tc.name: BackupRuleManager
* @tc.desc: backup rule manager.
* @tc.type: FUNC
*/
HWTEST_F(BackupRuleManagerTest, BackupRuleManager, TestSize.Level1)
{
    BackupRuleManagerTest::TestRule();
    std::vector<std::string> rule = { "TestRule" };
    BackupRuleManager::GetInstance().LoadBackupRules(rule);
    ASSERT_FALSE(BackupRuleManager::GetInstance().CanBackup());
}
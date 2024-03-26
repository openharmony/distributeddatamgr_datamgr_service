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

#define LOG_TAG "DataShareServiceImplTest"

#include <gtest/gtest.h>
#include "log_print.h"
#include "ipc_skeleton.h"
#include "data_share_service_impl.h"
#include "data_share_service_stub.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "app_connect_manager.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "data_ability_observer_interface.h"
#include "dataobs_mgr_client.h"
#include "datashare_errno.h"
#include "datashare_template.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "matching_skills.h"
#include "permit_delegate.h"
#include "scheduler_manager.h"
#include "subscriber_managers/published_data_subscriber_manager.h"
#include "template_data.h"
#include "utils/anonymous.h"

using namespace testing::ext;
using DumpManager = OHOS::DistributedData::DumpManager;
using namespace OHOS::DataShare;
namespace OHOS::Test {
class DataShareServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};

protected:
};

/**
* @tc.name: Insert001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    DataShareValuesBucket valuesBucket = {"key1": "value1", "key2": "value2"};
    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, ERROR);
}

/**
* @tc.name: Insert002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "test_uri";
    DataShareValuesBucket valuesBucket = {};
    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: Insert003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, Insert003, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "test_uri";
    DataShareValuesBucket valuesBucket = {"key1": "value1", "key2": "value2"};
    auto result = dataShareServiceImpl.Insert(uri, valuesBucket);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: NotifyChange001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, NotifyChange001, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "";
    auto result = dataShareServiceImpl.NotifyChange(uri);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: NotifyChange002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareServiceImplTest, NotifyChange002, TestSize.Level1)
{
    DataShareServiceImpl dataShareServiceImpl;
    std::string uri = "test_uri";
    auto result = dataShareServiceImpl.NotifyChange(uri);
    EXPECT_EQ(result, true);
}


} // namespace OHOS::Test
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

#define LOG_TAG "DataHandlerTest"

#include <gtest/gtest.h>
#include "data_handler.h"
#include "tlv_util.h"

namespace OHOS::UDMF {
using namespace testing::ext;
class DataHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: BuildEntries001
* @tc.desc: Normal test of BuildEntries, executors_ is nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(DataHandlerTest, BuildEntries001, TestSize.Level1)
{
    std::vector<std::shared_ptr<UnifiedRecord>> records = { nullptr };
    std::string unifiedKey = "unifiedKey";
    std::vector<Entry> entries;
    DataHandler dataHandler;
    Status status = dataHandler.BuildEntries(records, unifiedKey, entries);
    EXPECT_EQ(status, E_OK);
}
}; // namespace UDMF
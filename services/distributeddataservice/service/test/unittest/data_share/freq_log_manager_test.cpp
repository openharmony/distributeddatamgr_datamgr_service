/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "FreqLogManagerTest"

#include <gtest/gtest.h>

#include "freq_log_manager.h"
#include "idata_share_service.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS::DataShare;

namespace OHOS::Test {
class FreqLogManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
 * @tc.name: GetInstance_ReturnsSameInstance
 * @tc.desc: test FreqLogManager GetInstance returns the same singleton instance
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, GetInstance_ReturnsSameInstance, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::GetInstance_ReturnsSameInstance start");
    auto &instance1 = FreqLogManager::GetInstance();
    auto &instance2 = FreqLogManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
    ZLOGI("FreqLogManagerTest::GetInstance_ReturnsSameInstance end");
}

/**
 * @tc.name: ReportCall_ValidParams_NoFatal
 * @tc.desc: test ReportCall with valid QUERY code and URI does not crash
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, ReportCall_ValidParams_NoFatal, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::ReportCall_ValidParams_NoFatal start");
    auto &mgr = FreqLogManager::GetInstance();
    uint32_t code = IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY;
    uint64_t costMs = 5;
    std::string uri = "datashare:///com.example.app/module/store/table";
    EXPECT_NO_FATAL_FAILURE(mgr.ReportCall(code, costMs, uri));
    ZLOGI("FreqLogManagerTest::ReportCall_ValidParams_NoFatal end");
}

/**
 * @tc.name: ReportCall_EmptyUri_NoFatal
 * @tc.desc: test ReportCall with empty URI does not crash
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, ReportCall_EmptyUri_NoFatal, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::ReportCall_EmptyUri_NoFatal start");
    auto &mgr = FreqLogManager::GetInstance();
    uint32_t code = IDataShareService::DATA_SHARE_SERVICE_CMD_GET_SILENT_PROXY_STATUS;
    EXPECT_NO_FATAL_FAILURE(mgr.ReportCall(code, 10, ""));
    ZLOGI("FreqLogManagerTest::ReportCall_EmptyUri_NoFatal end");
}

/**
 * @tc.name: ReportCall_MultipleCalls_NoFatal
 * @tc.desc: test ReportCall with multiple calls for same caller does not crash
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, ReportCall_MultipleCalls_NoFatal, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::ReportCall_MultipleCalls_NoFatal start");
    auto &mgr = FreqLogManager::GetInstance();
    uint32_t code = IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY;
    std::string uri = "datashare:///com.target.app/module/store/table";
    for (int i = 0; i < 100; i++) {
        EXPECT_NO_FATAL_FAILURE(mgr.ReportCall(code, i, uri));
    }
    ZLOGI("FreqLogManagerTest::ReportCall_MultipleCalls_NoFatal end");
}

/**
 * @tc.name: ReportCall_DifferentCodes_NoFatal
 * @tc.desc: test ReportCall with both QUERY and GET_SILENT_PROXY_STATUS codes does not crash
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, ReportCall_DifferentCodes_NoFatal, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::ReportCall_DifferentCodes_NoFatal start");
    auto &mgr = FreqLogManager::GetInstance();
    std::string uri = "datashare:///com.multi.app/module/store/table";
    EXPECT_NO_FATAL_FAILURE(
        mgr.ReportCall(IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY, 5, uri));
    EXPECT_NO_FATAL_FAILURE(
        mgr.ReportCall(IDataShareService::DATA_SHARE_SERVICE_CMD_GET_SILENT_PROXY_STATUS, 3, uri));
    ZLOGI("FreqLogManagerTest::ReportCall_DifferentCodes_NoFatal end");
}

/**
 * @tc.name: SetThreadPool_Nullptr_NoFatal
 * @tc.desc: test SetThreadPool with nullptr does not crash
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(FreqLogManagerTest, SetThreadPool_Nullptr_NoFatal, TestSize.Level1)
{
    ZLOGI("FreqLogManagerTest::SetThreadPool_Nullptr_NoFatal start");
    auto &mgr = FreqLogManager::GetInstance();
    EXPECT_NO_FATAL_FAILURE(mgr.SetThreadPool(nullptr));
    ZLOGI("FreqLogManagerTest::SetThreadPool_Nullptr_NoFatal end");
}
} // namespace OHOS::Test
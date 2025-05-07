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

#include <gtest/gtest.h>

#include "bundle_mgr/bundle_mgr_adapter.h"
#include "ipc_skeleton.h"
namespace {
using namespace OHOS;
using namespace OHOS::DistributedData;
using namespace testing::ext;

class BundleMgrTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: GetBundleName
* @tc.desc: GetBundleName test
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(BundleMgrTest, GetBundleName, TestSize.Level0)
{
    auto bundleName = BundleMgrAdapter::GetInstance().GetBundleName(0);
    EXPECT_TRUE(bundleName.empty());
}
} // namespace
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
#include "delay_data_container.h"

using namespace OHOS::UDMF;
using namespace testing::ext;
using namespace testing;

namespace OHOS::Test {
namespace DistributedDataTest {
class UdmfDelayDataContainerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: DataLoadCallbackTest001
* @tc.desc: Test Execute data load callback with return false
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, DataLoadCallbackTest001, TestSize.Level1)
{
    auto container = DelayDataContainer::GetInstance();
    UnifiedData data;
    int32_t res = E_OK;
    QueryOption query;
    query.key = "";
    auto ret = container.HandleDelayLoad(query, data(), res);
    EXPECT_EQ(res, E_INVALID_PARAMETERS);
    EXPECT_EQ(ret, false);

    query.key = "udmf://drag/com.example.app/1233455";
    ret = container.HandleDelayLoad(query, data(), res);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: DataLoadCallbackTest002
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, DataLoadCallbackTest002, TestSize.Level1)
{
    auto container = DelayDataContainer::GetInstance();
    container.dataLoadCallback_.clear();
    std::string key = "";
    sptr<UdmfNotifierProxy> callback = nullptr;
    container.RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(container.dataLoadCallback_.empty());

    key = "udmf://drag/com.example.app/1233455";
    container.RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(container.dataLoadCallback_.empty());

    callback = new (std::nothrow) UdmfNotifierProxy();
    container.RegisterDataLoadCallback(key, callback);
    EXPECT_FALSE(container.dataLoadCallback_.empty());

    QueryOption query;
    query.key = key;
    UnifiedData data;
    int32_t res = E_OK;
    auto ret = container.HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    ret = container.HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    container.dataLoadCallback_.clear();
}

/**
* @tc.name: ExecDataLoadCallback001
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, ExecDataLoadCallback001, TestSize.Level1)
{
    auto container = DelayDataContainer::GetInstance();
    container.dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy();
    container.RegisterDataLoadCallback(key, callback);

    QueryOption query;
    query.key = key;
    auto ret = container.HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    ret = container.ExecDataLoadCallback(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    container.dataLoadCallback_.clear();
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
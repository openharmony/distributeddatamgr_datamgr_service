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
#include "config_factory.h"
#include "gtest/gtest.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
class ConfigFactoryTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
    }
    static void TearDownTestCase(void)
    {
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

/**
* @tc.name: GlobalConfig
* @tc.desc: load the config.json global info.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConfigFactoryTest, GlobalConfig, TestSize.Level0)
{
    auto *global = ConfigFactory::GetInstance().GetGlobalConfig();
    ASSERT_NE(global, nullptr);
    ASSERT_EQ(global->processLabel, "distributeddata");
    ASSERT_EQ(global->metaData, "service_meta");
    ASSERT_EQ(global->version, "000.000.001");
    std::vector<std::string> features{ "kvdb", "rdb", "object", "backup", "data_sync" };
    ASSERT_EQ(global->features, features);
}

/**
* @tc.name: ComponentConfig
* @tc.desc: load the config.json component info.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConfigFactoryTest, ComponentConfig, TestSize.Level0)
{
    auto *components = ConfigFactory::GetInstance().GetComponentConfig();
    ASSERT_NE(components, nullptr);
    ASSERT_EQ(components->size(), 2);
    const ComponentConfig &config = (*components)[0];
    ASSERT_EQ(config.description, "3rd party adapter");
    ASSERT_EQ(config.lib, "libconfigdemo.z.so");
    ASSERT_EQ(config.constructor, "");
    ASSERT_EQ(config.destructor, "");
    ASSERT_EQ(config.params, "{\"count\":1,\"key\":\"value\"}");
    const ComponentConfig &cfg = (*components)[1];
    ASSERT_EQ(cfg.lib, "libconfigdemo2.z.so");
}

/**
* @tc.name: CheckerConfig
* @tc.desc: load the config.json checkers info.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConfigFactoryTest, CheckerConfig, TestSize.Level0)
{
    auto *checker = ConfigFactory::GetInstance().GetCheckerConfig();
    ASSERT_NE(checker, nullptr);
    std::vector<std::string> checkers{"SystemChecker", "BundleChecker", "PackageChecker",
                                      "ExternalChecker"};
    ASSERT_EQ(checker->checkers, checkers);
    ASSERT_EQ(checker->trusts[0].bundleName, "bundle_manager_service");
    ASSERT_EQ(checker->trusts[0].appId, "bundle_manager_service");
    ASSERT_EQ(checker->trusts[0].checker, "SystemChecker");
}

/**
* @tc.name: NetworkConfig
* @tc.desc: load the config.json networks info.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConfigFactoryTest, NetworkConfig, TestSize.Level0)
{
    auto *networks = ConfigFactory::GetInstance().GetNetworkConfig();
    ASSERT_NE(networks, nullptr);
    std::vector<std::string> chains{ "loadBalance", "authentication", "traffic-control", "router", "transport",
        "fault-inject" };
    ASSERT_EQ(networks->chains, chains);
    std::vector<std::string> routers{ "OHOSRouter" };
    ASSERT_EQ(networks->routers, routers);
    std::vector<std::string> transports{ "softbus" };
    ASSERT_EQ(networks->transports, transports);
    ASSERT_EQ(networks->protocols[0].name, "OHOS softbus");
    ASSERT_EQ(networks->protocols[0].address, "ohos.distributeddata");
    ASSERT_EQ(networks->protocols[0].transport, "softbus");
}

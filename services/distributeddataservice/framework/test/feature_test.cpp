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
#include <gtest/gtest.h>

#include "feature/feature_system.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace DistributedDB {
struct AutoLaunchParam {
};
}

class FeatureSystemTest : public testing::Test {
};

class MockFeature : public FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, OHOS::MessageParcel& data, OHOS::MessageParcel& reply) override
    {
        return E_OK;
    }
};

/**
* @tc.name: GetInstanceTest
* @tc.desc: getInstance
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, GetInstanceTest, TestSize.Level1)
{
    FeatureSystem& featureSystem = FeatureSystem::GetInstance();
    ASSERT_NE(&featureSystem, nullptr);
}

/**
* @tc.name: RegisterCreatorTest And GetCreatorTest
* @tc.desc: registerCreatorTest And getCreator
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, RegisterCreatorAndGetCreatorTest, TestSize.Level1)
{
    FeatureSystem& featureSystem = FeatureSystem::GetInstance();
    std::string featureName = "MockFeature";
    FeatureSystem::Creator creator = []() {
        return std::shared_ptr<FeatureSystem::Feature>();
    };

    int32_t result = featureSystem.RegisterCreator(featureName, creator);
    EXPECT_EQ(result, E_OK);

    FeatureSystem::Creator registeredCreator = featureSystem.GetCreator(featureName);
    EXPECT_NE(registeredCreator, nullptr);
}

/**
* @tc.name: RegisterStaticActsTest And GetStaticActsTest
* @tc.desc: registerStaticActs And getStaticActs
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, GetStaticActsAndetStaticActsTest, TestSize.Level1)
{
    FeatureSystem& featureSystem = FeatureSystem::GetInstance();
    std::string staticActsName = "StaticActs";
    std::shared_ptr<StaticActs> staticActs = std::make_shared<StaticActs>();

    int32_t result = featureSystem.RegisterStaticActs(staticActsName, staticActs);
    EXPECT_EQ(result, E_OK);

    const OHOS::ConcurrentMap<std::string, std::shared_ptr<StaticActs>>& staticActsMap = featureSystem.GetStaticActs();
    auto [success, staticActsPtr] = staticActsMap.Find("StaticActs");
    EXPECT_NE(staticActsPtr, nullptr);
}

/**
* @tc.name: GetFeatureNameTest
* @tc.desc: getFeatureNameTest
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, GetFeatureNameTest, TestSize.Level1)
{
    FeatureSystem& featureSystem = FeatureSystem::GetInstance();
    std::string featureName1 = "Feature1";
    std::string featureName2 = "Feature2";

    featureSystem.RegisterCreator(featureName1, []() {
        return nullptr;
    });
    featureSystem.RegisterCreator(featureName2, []() {
        return nullptr;
    });

    std::vector<std::string> featureNames = featureSystem.GetFeatureName(FeatureSystem::BIND_LAZY);

    EXPECT_EQ(featureNames[0], featureName1);
    EXPECT_EQ(featureNames[1], featureName2);
}

/**
* @tc.name: OnInitializeTest
* @tc.desc: onInitializeTest
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, OnInitializeTest, TestSize.Level1)
{
    MockFeature feature;
    int32_t result = feature.OnInitialize();
    EXPECT_EQ(result, E_OK);
}

/**
* @tc.name: OnBindTest
* @tc.desc: onBind
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, OnBindTest, TestSize.Level1)
{
    MockFeature feature;
    FeatureSystem::Feature::BindInfo bindInfo;
    bindInfo.selfName = "Feature1";
    bindInfo.selfTokenId = 123;
    bindInfo.executors = std::shared_ptr<OHOS::ExecutorPool>();
    int32_t result = feature.OnBind(bindInfo);
    EXPECT_EQ(result, E_OK);
}

/**
* @tc.name: OnAppExitTest
* @tc.desc: onAppExitTest
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, OnAppExitTest, TestSize.Level1)
{
    MockFeature feature;
    pid_t uid = 1;
    pid_t pid = 2;
    uint32_t tokenId = 3;
    std::string bundleName = "com.example.app";
    int32_t result = feature.OnAppExit(uid, pid, tokenId, bundleName);
    EXPECT_EQ(result, E_OK);
}

/**
* @tc.name: FeatureTest001
* @tc.desc: onAppUninstallTest and onAppUpdate and onAppInstall
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, FeatureTest001, TestSize.Level1)
{
    FeatureSystem::Feature::BindInfo bindInfo;
    std::string bundleName = "com.example.app";
    int32_t user = 0;
    int32_t index = 1;

    MockFeature mockFeature;

    int32_t ret = mockFeature.OnAppInstall(bundleName, user, index);
    EXPECT_EQ(ret, E_OK);

    ret = mockFeature.OnAppUpdate(bundleName, user, index);
    EXPECT_EQ(ret, E_OK);

    ret = mockFeature.OnAppUninstall(bundleName, user, index);
    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: ResolveAutoLaunchTest
* @tc.desc: resolveAutoLaunch
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, ResolveAutoLaunchTest, TestSize.Level1)
{
    FeatureSystem::Feature::BindInfo bindInfo;
    std::string identifier = "example_identifier";
    DistributedDB::AutoLaunchParam param;

    MockFeature mockFeature;
    int32_t ret = mockFeature.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: OnUserChangeTest
* @tc.desc: onUserChange
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, OnUserChangeTest, TestSize.Level1)
{
    FeatureSystem::Feature::BindInfo bindInfo;
    uint32_t code = 1;
    std::string user = "example_user";
    std::string account = "example_account";

    MockFeature mockFeature;
    int32_t ret = mockFeature.OnUserChange(code, user, account);

    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: FeatureTest002
* @tc.desc: online and offline and onReady
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(FeatureSystemTest, FeatureTest002, TestSize.Level1)
{
    FeatureSystem::Feature::BindInfo bindInfo;
    std::string device = "example_device";

    MockFeature mockFeature;

    int32_t ret = mockFeature.Online(device);
    EXPECT_EQ(ret, E_OK);

    ret = mockFeature.Offline(device);
    EXPECT_EQ(ret, E_OK);

    ret = mockFeature.OnReady(device);
    EXPECT_EQ(ret, E_OK);
}

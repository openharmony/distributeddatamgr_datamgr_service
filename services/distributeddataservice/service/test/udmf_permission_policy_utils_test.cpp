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
#define LOG_TAG "UdmfPermissionPolicyUtilsTest"

#include <gtest/gtest.h>
#include <string>

#include "permission_policy_utils.h"
#include "unified_data.h"
#include "unified_meta.h"

using namespace testing::ext;
using namespace OHOS::UDMF;
using namespace OHOS;
namespace OHOS::Test {
using namespace std;

class UdmfPermissionPolicyUtilsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void UdmfPermissionPolicyUtilsTest::SetUpTestCase()
{
}

void UdmfPermissionPolicyUtilsTest::TearDownTestCase()
{
}

void UdmfPermissionPolicyUtilsTest::SetUp()
{
}

void UdmfPermissionPolicyUtilsTest::TearDown()
{
}

/**
* @tc.name: GetPermissionPolicyMode_001
* @tc.desc: Test GetPermissionPolicyMode with null runtime
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetPermissionPolicyMode_001, TestSize.Level1)
{
    std::shared_ptr<Runtime> runtime = nullptr;
    int32_t mode = PermissionPolicyUtils::GetPermissionPolicyMode(runtime);
    EXPECT_EQ(mode, PERMISSION_POLICY_MODE_LEGACY);
}

/**
* @tc.name: GetPermissionPolicyMode_002
* @tc.desc: Test GetPermissionPolicyMode with valid runtime
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetPermissionPolicyMode_002, TestSize.Level1)
{
    auto runtime = std::make_shared<Runtime>();
    runtime->permissionPolicyMode = PERMISSION_POLICY_MODE_MASK;
    int32_t mode = PermissionPolicyUtils::GetPermissionPolicyMode(runtime);
    EXPECT_EQ(mode, PERMISSION_POLICY_MODE_MASK);

    runtime->permissionPolicyMode = PERMISSION_POLICY_MODE_LEGACY;
    mode = PermissionPolicyUtils::GetPermissionPolicyMode(runtime);
    EXPECT_EQ(mode, PERMISSION_POLICY_MODE_LEGACY);
}

/**
* @tc.name: ConvertToGrantUriPermission_001
* @tc.desc: Test ConvertToGrantUriPermission with various masks
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ConvertToGrantUriPermission_001, TestSize.Level1)
{
    unsigned int result0 = PermissionPolicyUtils::ConvertToGrantUriPermission(0);
    EXPECT_EQ(result0, 0);

    unsigned int result1 = PermissionPolicyUtils::ConvertToGrantUriPermission(1);
    EXPECT_EQ(result1, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION);

    unsigned int result2 = PermissionPolicyUtils::ConvertToGrantUriPermission(2);
    EXPECT_EQ(result2, AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION);

    unsigned int result3 = PermissionPolicyUtils::ConvertToGrantUriPermission(3);
    EXPECT_EQ(result3, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
                     AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION);

    unsigned int result5 = PermissionPolicyUtils::ConvertToGrantUriPermission(5);
    EXPECT_EQ(result5, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
                     AAFwk::Want::FLAG_AUTH_PERSISTABLE_URI_PERMISSION);

    unsigned int result7 = PermissionPolicyUtils::ConvertToGrantUriPermission(7);
    EXPECT_EQ(result7, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
                     AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION |
                     AAFwk::Want::FLAG_AUTH_PERSISTABLE_URI_PERMISSION);

    unsigned int result4 = PermissionPolicyUtils::ConvertToGrantUriPermission(4);
    EXPECT_EQ(result4, 0);
}

/**
* @tc.name: ToPermissionMaskByLegacyPolicy_001
* @tc.desc: Test ToPermissionMaskByLegacyPolicy with various policies
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMaskByLegacyPolicy_001, TestSize.Level1)
{
    uint32_t result0 = PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(
        static_cast<int32_t>(PermissionPolicy::NO_PERMISSION));
    EXPECT_EQ(result0, 0);
    uint32_t result1 = PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(
        static_cast<int32_t>(PermissionPolicy::ONLY_READ));
    EXPECT_EQ(result1, UriPermissionUtil::READ_FLAG);
    uint32_t result2 = PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(
        static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    EXPECT_EQ(result2, UriPermissionUtil::READ_FLAG | UriPermissionUtil::WRITE_FLAG |
                    UriPermissionUtil::PERSIST_FLAG);
    uint32_t result3 = PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(
        static_cast<int32_t>(PermissionPolicy::UNKNOW));
    EXPECT_EQ(result3, UriPermissionUtil::READ_FLAG | UriPermissionUtil::WRITE_FLAG |
                    UriPermissionUtil::PERSIST_FLAG);
    uint32_t result4 = PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(100);
    EXPECT_EQ(result4, 0);
}

/**
* @tc.name: ToPermissionMask_001
* @tc.desc: Test ToPermissionMask with Object and legacy mode
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMask_001, TestSize.Level1)
{
    std::shared_ptr<Object> obj = nullptr;
    uint32_t result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, 0);
    obj = std::make_shared<Object>();
    result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, 0);
    obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(PermissionPolicy::ONLY_READ);
    result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, UriPermissionUtil::READ_FLAG);
}

/**
* @tc.name: ToPermissionMask_002
* @tc.desc: Test ToPermissionMask with Object and mask mode
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMask_002, TestSize.Level1)
{
    auto obj = std::make_shared<Object>();
    uint32_t result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_MASK);
    EXPECT_EQ(result, 0);
    obj->value_[URI_PERMISSION_MASK] = 5;
    result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_MASK);
    EXPECT_EQ(result, 5);
    obj->value_[URI_PERMISSION_MASK] = -1;
    result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_MASK);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: ToPermissionMask_003
* @tc.desc: Test ToPermissionMask with UriInfo and legacy mode
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMask_003, TestSize.Level1)
{
    UriInfo uriInfo;
    uriInfo.permission = static_cast<int32_t>(PermissionPolicy::ONLY_READ);
    uint32_t result = PermissionPolicyUtils::ToPermissionMask(uriInfo, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, UriPermissionUtil::READ_FLAG);
    uriInfo.permission = static_cast<int32_t>(PermissionPolicy::READ_WRITE);
    result = PermissionPolicyUtils::ToPermissionMask(uriInfo, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, UriPermissionUtil::READ_FLAG | UriPermissionUtil::WRITE_FLAG |
                    UriPermissionUtil::PERSIST_FLAG);
}

/**
* @tc.name: ToPermissionMask_004
* @tc.desc: Test ToPermissionMask with UriInfo and mask mode
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMask_004, TestSize.Level1)
{
    UriInfo uriInfo;
    uriInfo.permissionMask = 5;
    uint32_t result = PermissionPolicyUtils::ToPermissionMask(uriInfo, PERMISSION_POLICY_MODE_MASK);
    EXPECT_EQ(result, 5);
    uriInfo.permissionMask = 4;
    result = PermissionPolicyUtils::ToPermissionMask(uriInfo, PERMISSION_POLICY_MODE_MASK);
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: ToPermissionPolicyByLegacyMask_001
* @tc.desc: Test ToPermissionPolicyByLegacyMask with various masks
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionPolicyByLegacyMask_001, TestSize.Level1)
{
    int32_t result0 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(0);
    EXPECT_EQ(result0, static_cast<int32_t>(PermissionPolicy::NO_PERMISSION));
    int32_t result1 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(1);
    EXPECT_EQ(result1, static_cast<int32_t>(PermissionPolicy::ONLY_READ));
    int32_t result2 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(2);
    EXPECT_EQ(result2, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    int32_t result3 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(3);
    EXPECT_EQ(result3, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    int32_t result4 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(4);
    EXPECT_EQ(result4, static_cast<int32_t>(PermissionPolicy::NO_PERMISSION));
    int32_t result5 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(5);
    EXPECT_EQ(result5, static_cast<int32_t>(PermissionPolicy::NO_PERMISSION));
    int32_t result6 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(6);
    EXPECT_EQ(result6, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
    int32_t result7 = PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(7);
    EXPECT_EQ(result7, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
}

/**
* @tc.name: ToPermissionPolicy_001
* @tc.desc: Test ToPermissionPolicy
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionPolicy_001, TestSize.Level1)
{
    int32_t result0 = PermissionPolicyUtils::ToPermissionPolicy(0);
    EXPECT_EQ(result0, static_cast<int32_t>(PermissionPolicy::NO_PERMISSION));
    int32_t result1 = PermissionPolicyUtils::ToPermissionPolicy(1);
    EXPECT_EQ(result1, static_cast<int32_t>(PermissionPolicy::ONLY_READ));
    int32_t result2 = PermissionPolicyUtils::ToPermissionPolicy(2);
    EXPECT_EQ(result2, static_cast<int32_t>(PermissionPolicy::READ_WRITE));
}

/**
* @tc.name: GetPropertiesUriAuthorizationMask_001
* @tc.desc: Test GetPropertiesUriAuthorizationMask with null properties
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetPropertiesUriAuthorizationMask_001, TestSize.Level1)
{
    std::shared_ptr<UnifiedDataProperties> properties = nullptr;
    uint32_t mask = 0;
    bool result = PermissionPolicyUtils::GetPropertiesUriAuthorizationMask(properties, mask);
    EXPECT_FALSE(result);
    EXPECT_EQ(mask, 0);
}

/**
* @tc.name: GetPropertiesUriAuthorizationMask_002
* @tc.desc: Test GetPropertiesUriAuthorizationMask with empty policies
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetPropertiesUriAuthorizationMask_002, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    uint32_t mask = 0;
    bool result = PermissionPolicyUtils::GetPropertiesUriAuthorizationMask(properties, mask);
    EXPECT_FALSE(result);
    EXPECT_EQ(mask, 0);
}

/**
* @tc.name: GetPropertiesUriAuthorizationMask_003
* @tc.desc: Test GetPropertiesUriAuthorizationMask with valid policies
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetPropertiesUriAuthorizationMask_003, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->uriAuthorizationPolicies = {UriPermission::READ, UriPermission::WRITE};

    uint32_t mask = 0;
    bool result = PermissionPolicyUtils::GetPropertiesUriAuthorizationMask(properties, mask);
    EXPECT_TRUE(result);
    EXPECT_EQ(mask, 3);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_001
* @tc.desc: Test GetAuthorizedUriPermissionMask with no available permissions
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_001, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    auto recordObject = std::make_shared<Object>();
    uint32_t availableMask = 0;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_FALSE(result);
    EXPECT_EQ(grantMask, 0);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_002
* @tc.desc: Test GetAuthorizedUriPermissionMask with only available permissions
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_002, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    auto recordObject = std::make_shared<Object>();

    uint32_t availableMask = 7;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_TRUE(result);
    EXPECT_EQ(grantMask, 7);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_003
* @tc.desc: Test GetAuthorizedUriPermissionMask with properties permissions
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_003, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->uriAuthorizationPolicies = {UriPermission::READ};
    auto recordObject = std::make_shared<Object>();

    uint32_t availableMask = 7;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_TRUE(result);
    EXPECT_EQ(grantMask, 1);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_004
* @tc.desc: Test GetAuthorizedUriPermissionMask with record permissions
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_004, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    auto recordObject = std::make_shared<Object>();
    recordObject->value_[URI_AUTHORIZATION_POLICIES] = 2;

    uint32_t availableMask = 7;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_TRUE(result);
    EXPECT_EQ(grantMask, 2);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_005
* @tc.desc: Test GetAuthorizedUriPermissionMask with no intersection
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_005, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->uriAuthorizationPolicies = {UriPermission::WRITE};
    auto recordObject = std::make_shared<Object>();

    uint32_t availableMask = 1;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_FALSE(result);
    EXPECT_EQ(grantMask, 0);
}

/**
* @tc.name: AppendGrantUriPermission_001
* @tc.desc: Test AppendGrantUriPermission with empty input
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, AppendGrantUriPermission_001, TestSize.Level1)
{
    std::map<std::string, uint32_t> strUris;
    std::map<std::string, unsigned int> uriPermissions;

    PermissionPolicyUtils::AppendGrantUriPermission(strUris, uriPermissions);
    EXPECT_TRUE(uriPermissions.empty());
}

/**
* @tc.name: AppendGrantUriPermission_002
* @tc.desc: Test AppendGrantUriPermission with valid URIs
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, AppendGrantUriPermission_002, TestSize.Level1)
{
    std::map<std::string, uint32_t> strUris = {
        {"file://data1.txt", 1},
        {"file://data2.txt", 2},
        {"file://data3.txt", 5}
    };
    std::map<std::string, unsigned int> uriPermissions;
    PermissionPolicyUtils::AppendGrantUriPermission(strUris, uriPermissions);
    EXPECT_EQ(uriPermissions.size(), 3);
}

/**
* @tc.name: AppendGrantUriPermission_004
* @tc.desc: Test AppendGrantUriPermission with duplicate URIs
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, AppendGrantUriPermission_004, TestSize.Level1)
{
    std::map<std::string, uint32_t> strUris = {
        {"file://data1.txt", 3}
    };
    std::map<std::string, unsigned int> uriPermissions;

    PermissionPolicyUtils::AppendGrantUriPermission(strUris, uriPermissions);
    EXPECT_EQ(uriPermissions.size(), 1);
    EXPECT_EQ(uriPermissions["file://data1.txt"],
        AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
        AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION);
}

/**
* @tc.name: AppendGrantUriPermission_005
* @tc.desc: Test AppendGrantUriPermission with empty URI string
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, AppendGrantUriPermission_005, TestSize.Level1)
{
    std::map<std::string, uint32_t> strUris = {
        {"", 1},
        {"file://data1.txt", 2}
    };
    std::map<std::string, unsigned int> uriPermissions;
    PermissionPolicyUtils::AppendGrantUriPermission(strUris, uriPermissions);
    EXPECT_EQ(uriPermissions.size(), 1);
    EXPECT_EQ(uriPermissions.count(""), 0);
}

/**
* @tc.name: AppendGrantUriPermission_006
* @tc.desc: Test AppendGrantUriPermission with zero available mask
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, AppendGrantUriPermission_006, TestSize.Level1)
{
    std::map<std::string, uint32_t> strUris = {
        {"file://data1.txt", 0},
        {"file://data2.txt", 2}
    };
    std::map<std::string, unsigned int> uriPermissions;
    PermissionPolicyUtils::AppendGrantUriPermission(strUris, uriPermissions);
    EXPECT_EQ(uriPermissions.size(), 1);
    EXPECT_EQ(uriPermissions["file://data2.txt"],
        AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION);
}

/**
* @tc.name: ToPermissionMask_005
* @tc.desc: Test ToPermissionMask with Object and legacy mode with valid permission
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, ToPermissionMask_005, TestSize.Level1)
{
    auto obj = std::make_shared<Object>();
    obj->value_[PERMISSION_POLICY] = static_cast<int32_t>(PermissionPolicy::ONLY_READ);
    uint32_t result = PermissionPolicyUtils::ToPermissionMask(obj, PERMISSION_POLICY_MODE_LEGACY);
    EXPECT_EQ(result, UriPermissionUtil::READ_FLAG);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_006
* @tc.desc: Test GetAuthorizedUriPermissionMask with invalid record permission
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_006, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    properties->uriAuthorizationPolicies = {UriPermission::READ};
    auto recordObject = std::make_shared<Object>();
    recordObject->value_[URI_AUTHORIZATION_POLICIES] = -1;

    uint32_t availableMask = 7;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_TRUE(result);
    EXPECT_EQ(grantMask, 1);
}

/**
* @tc.name: GetAuthorizedUriPermissionMask_007
* @tc.desc: Test GetAuthorizedUriPermissionMask with no properties or record permission
* @tc.type: FUNC
*/
HWTEST_F(UdmfPermissionPolicyUtilsTest, GetAuthorizedUriPermissionMask_007, TestSize.Level1)
{
    auto properties = std::make_shared<UnifiedDataProperties>();
    auto recordObject = std::make_shared<Object>();

    uint32_t availableMask = 7;
    uint32_t grantMask = 0;
    bool result = PermissionPolicyUtils::GetAuthorizedUriPermissionMask(
        properties, recordObject, availableMask, grantMask);
    EXPECT_TRUE(result);
    EXPECT_EQ(grantMask, 7);
}

} // OHOS::Test

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
#define LOG_TAG "DataShareSaConnectionTest"
#include <gtest/gtest.h>

#include "data_share_sa_connection.h"
#include "datashare_errno.h"
#include "if_local_ability_manager.h"
#include "if_system_ability_manager.h"
#include "system_ability_load_callback_stub.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS;
constexpr uint32_t WAIT_TIME = 10;
constexpr int32_t SA_ID = 1001;
constexpr int32_t INTERFACE_CODE = 100;
namespace OHOS::Test {
class SAConnectionTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class RemoteObjectTest : public IRemoteObject {
public:
    explicit RemoteObjectTest(std::u16string descriptor) : IRemoteObject(descriptor) {}
    ~RemoteObjectTest() {}

    int32_t GetObjectRefCount()
    {
        return 0;
    }
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
    {
        return 0;
    }
    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }
    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient)
    {
        return true;
    }
    int Dump(int fd, const std::vector<std::u16string> &args)
    {
        return 0;
    }
};

/**
 * @tc.name: SAConnectionConstructor
 * @tc.desc: Verify constructor of SAConnection class correctly initializes member variables
 *           with valid SA ID and wait time parameters.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SAConnection objects without initialization errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001) and WAIT_TIME (wait time = 10).
 *     3. The SAConnection class has public members systemAbilityId_ and waitTime_
          that can be accessed for verification.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Verify that systemAbilityId_ member of connection object equals SA_ID.
 *     3. Verify that waitTime_ member of connection object equals WAIT_TIME.
 * @tc.expect:
 *     1. The connection.systemAbilityId_ equals SA_ID (1001).
 *     2. The connection.waitTime_ equals WAIT_TIME (10).
 */
HWTEST_F(SAConnectionTest, SAConnectionConstructor, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    EXPECT_EQ(connection.systemAbilityId_, SA_ID);
    EXPECT_EQ(connection.waitTime_, WAIT_TIME);
}

/**
 * @tc.name: SAConnectionInputParaSet
 * @tc.desc: Verify InputParaSet method of SAConnection class correctly sets input parameters
 *           from a MessageParcel.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.preconcon:
 *     1. The test environment supports instantiation of SAConnection and MessageParcel objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001) and WAIT_TIME (wait time = 10).
 *     3. The SAConnection class provides an InputParaSet method that accepts a MessageParcel reference.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Create an empty MessageParcel object (data).
 *     3. Call InputParaSet method on connection object, passing data parcel.
 *     4. Verify that method returns true, indicating successful parameter setting.
 * @tc.expect:
 *     1. The InputParaSet method returns true, indicating successful input parameter setting.
 */
HWTEST_F(SAConnectionTest, SAConnectionInputParaSet, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    MessageParcel data;
    bool result = connection.InputParaSet(data);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SAConnectionOutputParaGet
 * @tc.desc: Verify OutputParaGet method of SAConnection class correctly retrieves output parameters
 *           from a MessageParcel and updates member variables.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SAConnection and MessageParcel objects.
 *     2. Predefined constants are: valid: SA_ID (system ability ID = 1001), WAIT_TIME (wait time = 10),
 *        and INTERFACE_CODE (interface code = 100).
 *     3. The SAConnection class provides an OutputParaGet method that accepts a MessageParcel reference.
 *     4. The SAConnection class has public members code_ and descriptor_ that can be accessed.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Create a MessageParcel object (reply) and write an interface token (u"test_descriptor") and
 *        INTERFACE_CODE to it.
 *     3. Call OutputParaGet method on connection object, passing reply parcel.
 *     4. Verify that method returns true, indicating successful parameter retrieval.
 *     5. Verify that code_ member of connection object equals INTERFACE_CODE.
 *     6. Verify that descriptor_ member of connection object equals u"test_descriptor".
 * @tc.expect:
 *     1. The OutputParaGet method returns true, indicating successful output parameter retrieval.
 *     2. The connection.code_ equals INTERFACE_CODE (100).
 *     3. The connection.descriptor_ equals u"test_descriptor".
 */
HWTEST_F(SAConnectionTest, SAConnectionOutputParaGet, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    MessageParcel reply;
    reply.WriteInterfaceToken(u"test_descriptor");
    reply.WriteInt32(INTERFACE_CODE);
    
    bool result = connection.OutputParaGet(reply);
    EXPECT_TRUE(result);
    EXPECT_EQ(connection.code_, INTERFACE_CODE);
    EXPECT_EQ(connection.descriptor_, u"test_descriptor");
}

/**
 * @tc.name: SAConnectionSALoadCallbackOnLoadSystemAbilitySuccess
 * @tc.desc: Verify OnLoadSystemAbilitySuccess method of SALoadCallback class correctly
 *           updates isLoadSuccess_ flag based on provided remote object.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SALoadCallback and RemoteObjectTest objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001).
 *     3. The SALoadCallback class has a public isLoadSuccess_ member that can be accessed for verification.
 *     4. The RemoteObjectTest class is a mock implementation of IRemoteObject for testing purposes.
 * @tc.step:
 *     1. Instantiate a SALoadCallback object.
 *     2. Call OnLoadSystemAbilitySuccess with SA_ID and nullptr to simulate failed system ability loading.
 *     3. Verify that isLoadSuccess_ is false.
 *     4. Create a valid RemoteObjectTest object with descriptor u"OHOS.DataShare.IDataShare".
 *     5. Call OnLoadSystemAbilitySuccess again with SA_ID and valid token.
 *     6. Verify that isLoadSuccess_ is true.
 * @tc.expect:
 *     1. After first call with nullptr, callback.isLoadSuccess_ is false.
 *     2. After second call with a valid token, callback.isLoadSuccess_ is true.
 */
HWTEST_F(SAConnectionTest, SAConnectionSALoadCallbackOnLoadSystemAbilitySuccess, TestSize.Level1)
{
    SAConnection::SALoadCallback callback;
    
    callback.OnLoadSystemAbilitySuccess(SA_ID, nullptr);
    EXPECT_FALSE(callback.isLoadSuccess_.load());
    std::u16string tokenString = u"OHOS.DataShare.IDataShare";
    sptr<IRemoteObject> token = new (std::nothrow) RemoteObjectTest(tokenString);
    callback.OnLoadSystemAbilitySuccess(SA_ID, token);
    EXPECT_TRUE(callback.isLoadSuccess_.load());
}

/**
 * @tc.name: SAConnectionSALoadCallback_OnLoadSystemAbilityFail
 * @tc.desc: Verify OnLoadSystemAbilityFail method of SALoadCallback class correctly
 *           sets isLoadSuccess_ flag to false when system ability loading fails.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SALoadCallback objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001).
 *     3. The SALoadCallback class has a public isLoadSuccess_ member that can be accessed for verification.
 *     4. The SALoadCallback class provides an OnLoadSystemAbilityFail method that should set isLoadSuccess_ to false.
 * @tc.step:
 *     1. Instantiate a SALoadCallback object.
 *     2. Call OnLoadSystemAbilityFail with SA_ID to simulate failed system ability loading.
 *     3. Verify that isLoadSuccess_ is false.
 * @tc.expect:
 *     1. After calling OnLoadSystemAbilityFail, callback.isLoadSuccess_ is false.
 */
HWTEST_F(SAConnectionTest, SAConnectionSALoadCallback_OnLoadSystemAbilityFail, TestSize.Level1)
{
    SAConnection::SALoadCallback callback;
    
    callback.OnLoadSystemAbilityFail(SA_ID);
    EXPECT_FALSE(callback.isLoadSuccess_.load());
}

/**
 * @tc.name: SAConnectionGetConnectionInterfaceInfo
 * @tc.desc: Verify GetConnectionInterfaceInfo method of SAConnection class returns
 *           E_SYSTEM_ABILITY_OPERATE_FAILED when no valid system ability is available.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SAConnection objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001) and WAIT_TIME (wait time = 10).
 *     3. The SAConnection class provides a GetConnectionInterfaceInfo method
          that returns a pair<int32_t, std::u16string>.
 *     4. The E_SYSTEM_ABILITY_OPERATE_FAILED constant is predefined and accessible.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Call GetConnectionInterfaceInfo method on connection object to retrieve interface info.
 *     3. Verify that first element of returned pair equals E_SYSTEM_ABILITY_OPERATE_FAILED.
 * @tc.expect:
 *     1. The result.first equals E_SYSTEM_ABILITY_OPERATE_FAILED, indicating system ability operation failed.
 */
HWTEST_F(SAConnectionTest, SAConnectionGetConnectionInterfaceInfo, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    auto result = connection.GetConnectionInterfaceInfo();
    EXPECT_EQ(result.first, E_SYSTEM_ABILITY_OPERATE_FAILED);
}

/**
 * @tc.name: SAConnectionCheckAndLoadSystemAbility
 * @tc.desc: Verify CheckAndLoadSystemAbility method of SAConnection class returns false
 *           when system ability check and loading fails.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SAConnection objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001) and WAIT_TIME (wait time = 10).
 *     3. The SAConnection class provides a CheckAndLoadSystemAbility method that returns a boolean.
 *     4. The test environment has no valid system ability registered for given SA_ID.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Call CheckAndLoadSystemAbility method on connection object to check and load system ability.
 *     3. Verify that method returns false, indicating system ability check and loading failed.
 * @tc.expect:
 *     1. The CheckAndLoadSystemAbility method returns false, indicating system ability check and loading failed.
 */
HWTEST_F(SAConnectionTest, SAConnectionCheckAndLoadSystemAbility, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    bool result = connection.CheckAndLoadSystemAbility();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SAConnectionGetLocalAbilityManager
 * @tc.desc: Verify GetLocalAbilityManager method of SAConnection class returns nullptr
 *           when no valid local ability manager is available.
 * @tc.type: FUNC
 * @tc.require: issueIC8OCN
 * @tc.precon:
 *     1. The test environment supports instantiation of SAConnection objects without errors.
 *     2. Predefined constants are valid: SA_ID (system ability ID = 1001) and waitTime (wait time = 10).
 *     3. The SAConnection class provides a GetLocalAbilityManager method that returns a sptr<ILocalAbilityManager>.
 *     4. The test environment has no valid local ability manager registered.
 * @tc.step:
 *     1. Instantiate a SAConnection object by passing SA_ID and WAIT_TIME as constructor parameters.
 *     2. Call GetLocalAbilityManager method on connection object to retrieve local ability manager.
 *     3. Verify that returned result is nullptr.
 * @tc.expect:
 *     1. The GetLocalAbilityManager method returns nullptr, indicating no valid local ability manager is available.
 */
HWTEST_F(SAConnectionTest, SAConnectionGetLocalAbilityManager, TestSize.Level1)
{
    SAConnection connection(SA_ID, WAIT_TIME);
    auto result = connection.GetLocalAbilityManager();
    EXPECT_EQ(result, nullptr);
}
} // namespace OHOS::Test

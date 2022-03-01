/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>

#include "communicator_proxy.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "mock_communicator.h"
#include "platform_specific.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_store_sync_test";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const std::string DEVICE_D = "deviceD";
    const std::string DEVICE_E = "deviceE";


    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvDelegatePtr = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
}

class DistributedDBCommunicatorProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    MockCommunicator extComm_;
    MockCommunicator mainComm_;
    CommunicatorProxy *commProxy_ = nullptr;
};

void DistributedDBCommunicatorProxyTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }

    auto communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator);
}

void DistributedDBCommunicatorProxyTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBCommunicatorProxyTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: Get a KvStoreNbDelegate and init the CommunicatorProxy
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    std::string identifier = g_mgr.GetKvStoreIdentifier(USER_ID, APP_ID, STORE_ID);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    commProxy_ = new (std::nothrow) CommunicatorProxy();
    ASSERT_TRUE(commProxy_ != nullptr);
    commProxy_->SetMainCommunicator(&mainComm_);
    commProxy_->SetEqualCommunicator(&extComm_, identifier, { DEVICE_C });
}

void DistributedDBCommunicatorProxyTest::TearDown(void)
{
    /**
     * @tc.teardown: Release the KvStoreNbDelegate and CommunicatorProxy
     */
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    if (commProxy_ != nullptr) {
        RefObject::DecObjRef(commProxy_);
    }
    commProxy_ = nullptr;
}

/**
 * @tc.name: Interface set equal 001
 * @tc.desc: Test set equal identifier from interface.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, InterfaceSetEqualId001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call GetKvStoreIdentifier to make a store identifier.
     */
    std::string identifier = g_mgr.GetKvStoreIdentifier("default", APP_ID, STORE_ID);

    /**
     * @tc.steps: step2. Call SetEqualIdentifier to set the store identifier B, D, E.
     * @tc.expected: step2. SetEqualIdentifier return OK.
     */
    DBStatus status = g_kvDelegatePtr->SetEqualIdentifier(identifier, { DEVICE_B, DEVICE_D, DEVICE_E });
    EXPECT_EQ(status, DBStatus::OK);

    /**
     * @tc.steps: step2. Call SetEqualIdentifier to set the store identifier B.
     * @tc.expected: step2. SetEqualIdentifier return OK and D, E will offline.
     */
    status = g_kvDelegatePtr->SetEqualIdentifier(identifier, { DEVICE_B });
    EXPECT_EQ(status, DBStatus::OK);
}

/**
 * @tc.name: Register callback 001
 * @tc.desc: Test register callback from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, RegCallBack001, TestSize.Level1)
{
    OnMessageCallback msgCallback;
    OnConnectCallback connCallback;
    std::function<void(void)> sendableCallback;
    Finalizer finalizer;

    /**
     * @tc.steps: step1. Call RegOnMessageCallback from CommProxy.
     * @tc.expected: step1. mainComm and extComm's RegOnMessageCallback should be called once.
     */
    EXPECT_CALL(extComm_, RegOnMessageCallback(_, _)).Times(1);
    EXPECT_CALL(mainComm_, RegOnMessageCallback(_, _)).Times(1);
    commProxy_->RegOnMessageCallback(msgCallback, finalizer);

    /**
     * @tc.steps: step2. Call RegOnConnectCallback from CommProxy.
     * @tc.expected: step2. mainComm and extComm's RegOnConnectCallback should be called once.
     */
    EXPECT_CALL(extComm_, RegOnConnectCallback(_, _)).Times(1);
    EXPECT_CALL(mainComm_, RegOnConnectCallback(_, _)).Times(1);
    commProxy_->RegOnConnectCallback(connCallback, finalizer);

    /**
     * @tc.steps: step3. Call RegOnSendableCallback from CommProxy.
     * @tc.expected: step3. mainComm and extComm's RegOnSendableCallback should be called once.
     */
    EXPECT_CALL(extComm_, RegOnSendableCallback(_, _)).Times(1);
    EXPECT_CALL(mainComm_, RegOnSendableCallback(_, _)).Times(1);
    commProxy_->RegOnSendableCallback(sendableCallback, finalizer);
}

/**
 * @tc.name: Activate 001
 * @tc.desc: Test Activate called from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, Activate001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call Activate from CommProxy.
     * @tc.expected: step1. mainComm and extComm's Activate should be called once.
     */
    EXPECT_CALL(extComm_, Activate()).Times(1);
    EXPECT_CALL(mainComm_, Activate()).Times(1);
    commProxy_->Activate();
}

/**
 * @tc.name: Get mtu 001
 * @tc.desc: Test mtu called from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, GetMtu001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call GetCommunicatorMtuSize from CommProxy with no param.
     * @tc.expected: step1. GetCommunicatorMtuSize return DBConstant::MIN_MTU_SIZE.
     */
    EXPECT_CALL(mainComm_, GetCommunicatorMtuSize()).WillOnce(Return(DBConstant::MIN_MTU_SIZE));
    EXPECT_EQ(commProxy_->GetCommunicatorMtuSize(), DBConstant::MIN_MTU_SIZE);

    /**
     * @tc.steps: step2. Call GetCommunicatorMtuSize from CommProxy with param DEVICE_C.
     * @tc.expected: step2. GetCommunicatorMtuSize return DBConstant::MAX_MTU_SIZE.
     */
    EXPECT_CALL(extComm_, GetCommunicatorMtuSize(DEVICE_C)).WillOnce(Return(DBConstant::MAX_MTU_SIZE));
    EXPECT_EQ(commProxy_->GetCommunicatorMtuSize(DEVICE_C), DBConstant::MAX_MTU_SIZE);
}

/**
 * @tc.name: Get local identify 001
 * @tc.desc: Test Get local identify from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, GetLocalIdentity001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call GetLocalIdentity from CommProxy, and set mainComm return DEVICE_B.
     * @tc.expected: step1. GetCommunicatorMtuSize return DEVICE_B and function call return E_OK.
     */
    EXPECT_CALL(mainComm_, GetLocalIdentity(_)).WillOnce(DoAll(SetArgReferee<0>(DEVICE_B), Return(E_OK)));
    std::string localId;
    EXPECT_EQ(commProxy_->GetLocalIdentity(localId), E_OK);
    EXPECT_EQ(localId, DEVICE_B);
}

/**
 * @tc.name: Get remote version 001
 * @tc.desc: Test Get remote version from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, GetRemoteVersion001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set mainComm called GetRemoteCommunicatorVersion will return SOFTWARE_VERSION_BASE.
     */
    EXPECT_CALL(mainComm_, GetRemoteCommunicatorVersion(DEVICE_B, _))
        .WillOnce(DoAll(SetArgReferee<1>(SOFTWARE_VERSION_BASE), Return(E_OK)));

    /**
     * @tc.steps: step2. Call GetRemoteCommunicatorVersion from CommProxy with param DEVICE_B.
     * @tc.expected: step2. GetRemoteCommunicatorVersion return SOFTWARE_VERSION_BASE and function call return E_OK.
     */
    uint16_t version = 0;
    EXPECT_EQ(commProxy_->GetRemoteCommunicatorVersion(DEVICE_B, version), E_OK);
    EXPECT_EQ(version, SOFTWARE_VERSION_BASE);

    /**
     * @tc.steps: step3. Set extComm called GetRemoteCommunicatorVersion will return SOFTWARE_VERSION_CURRENT.
     */
    EXPECT_CALL(extComm_, GetRemoteCommunicatorVersion(DEVICE_C, _))
        .WillOnce(DoAll(SetArgReferee<1>(SOFTWARE_VERSION_CURRENT), Return(E_OK)));

    /**
     * @tc.steps: step4. Call GetRemoteCommunicatorVersion from CommProxy with param DEVICE_C.
     * @tc.expected: step4. GetRemoteCommunicatorVersion return SOFTWARE_VERSION_CURRENT and function call return E_OK.
     */
    EXPECT_EQ(commProxy_->GetRemoteCommunicatorVersion(DEVICE_C, version), E_OK);
    EXPECT_EQ(version, SOFTWARE_VERSION_CURRENT);
}

/**
 * @tc.name: Send message 001
 * @tc.desc: Test Send message from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, SendMessage001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call SendMessage from CommProxy with param DEVICE_B.
     * @tc.expected: step1. MainComm's SendMessage willed called and return E_OK.
     */
    SendConfig conf = {true, 0};
    EXPECT_CALL(mainComm_, SendMessage(DEVICE_B, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(commProxy_->SendMessage(DEVICE_B, nullptr, conf, nullptr), E_OK);

    /**
     * @tc.steps: step1. Call SendMessage from CommProxy with param DEVICE_C.
     * @tc.expected: step1. ExtComm's SendMessage willed called and return E_OK.
     */
    EXPECT_CALL(extComm_, SendMessage(DEVICE_C, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(commProxy_->SendMessage(DEVICE_C, nullptr, conf, nullptr), E_OK);
}

/**
 * @tc.name: Get timeout time 001
 * @tc.desc: Test get timout called from CommunicatorProxy.
 * @tc.type: FUNC
 * @tc.require: AR000F4GVG
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBCommunicatorProxyTest, GetTimeout001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call GetTimeout from CommProxy with no param.
     * @tc.expected: step1. GetTimeout return DBConstant::MIN_TIMEOUT.
     */
    EXPECT_CALL(mainComm_, GetTimeout()).WillOnce(Return(DBConstant::MIN_TIMEOUT));
    EXPECT_EQ(commProxy_->GetTimeout(), DBConstant::MIN_TIMEOUT);

    /**
     * @tc.steps: step2. Call GetTimeout from CommProxy with param DEVICE_C.
     * @tc.expected: step2. GetTimeout return DBConstant::MAX_MTU_SIZE.
     */
    EXPECT_CALL(extComm_, GetTimeout(DEVICE_C)).WillOnce(Return(DBConstant::MAX_TIMEOUT));
    EXPECT_EQ(commProxy_->GetTimeout(DEVICE_C), DBConstant::MAX_TIMEOUT);
}

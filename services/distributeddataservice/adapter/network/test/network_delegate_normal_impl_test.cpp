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

#include "network_delegate.h"
#include "network_delegate_normal_impl.h"
#include "network_delegate_normal_impl.cpp"
#include <gtest/gtest.h>
#include <unistd.h>

using namespace testing::ext;
using namespace std;
using namespace OHOS::DistributedData;
using namespace OHOS::NetManagerStandard;
using DmDeviceInfo = OHOS::DistributedHardware::DmDeviceInfo;
namespace OHOS::Test {
namespace DistributedDataTest {
class NetworkDelegateNormalImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void NetworkDelegateNormalImplTest::SetUpTestCase(void)
{
}

void NetworkDelegateNormalImplTest::TearDownTestCase()
{
}

void NetworkDelegateNormalImplTest::SetUp()
{
}

void NetworkDelegateNormalImplTest::TearDown()
{
}

/**
* @tc.name: GetNetworkType001
* @tc.desc: GetNetworkType testing exception branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, GetNetworkType001, TestSize.Level1)
{
    NetworkDelegateNormalImpl delegate;
    bool retrieve = false;
    EXPECT_NO_FATAL_FAILURE(delegate.RegOnNetworkChange());
    NetworkDelegate::NetworkType status = delegate.GetNetworkType(retrieve);
    EXPECT_EQ(status, NetworkDelegate::NetworkType::NONE);
}

/**
* @tc.name: GetNetworkType002
* @tc.desc: GetNetworkType testing normal branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, GetNetworkType002, TestSize.Level1)
{
    NetworkDelegateNormalImpl delegate;
    bool retrieve = true;
    EXPECT_NO_FATAL_FAILURE(delegate.RegOnNetworkChange());
    NetworkDelegate::NetworkType status = delegate.GetNetworkType(retrieve);
    EXPECT_EQ(status, NetworkDelegate::NetworkType::NONE);
}

/**
* @tc.name: IsNetworkAvailable
* @tc.desc: IsNetworkAvailable testing different branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, IsNetworkAvailable, TestSize.Level1)
{
    NetworkDelegateNormalImpl delegate;
    bool ret = delegate.IsNetworkAvailable(); // false false
    EXPECT_FALSE(ret);

    DmDeviceInfo& info = const_cast<DmDeviceInfo &>(delegate.cloudDmInfo_);
    std::fill(info.networkId, info.networkId + sizeof(info.networkId), '\0');
    NetworkDelegateNormalImpl::NetworkType netWorkType = NetworkDelegate::NetworkType::NONE;
    NetworkDelegateNormalImpl::NetworkType status = delegate.SetNet(netWorkType);
    EXPECT_EQ(status, NetworkDelegate::NONE);
    ret = delegate.IsNetworkAvailable(); // false true
    EXPECT_FALSE(ret);

    netWorkType = NetworkDelegate::NetworkType::WIFI;
    status = delegate.SetNet(netWorkType);
    EXPECT_EQ(status, NetworkDelegate::WIFI);
    ret = delegate.IsNetworkAvailable(); // true true
    EXPECT_TRUE(ret);

    netWorkType = NetworkDelegate::NetworkType::NONE;
    status = delegate.SetNet(netWorkType);
    EXPECT_EQ(status, NetworkDelegate::NONE);
    ret = delegate.IsNetworkAvailable(); // false true
    EXPECT_FALSE(ret);

    netWorkType = NetworkDelegate::NetworkType::WIFI;
    status = delegate.SetNet(netWorkType);
    EXPECT_EQ(status, NetworkDelegate::WIFI);
    ret = delegate.IsNetworkAvailable(); // false true
    EXPECT_TRUE(ret);
}

/**
* @tc.name: NetCapabilitiesChange001
* @tc.desc: NetCapabilitiesChange testing different branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, NetCapabilitiesChange001, TestSize.Level1)
{
    NetworkDelegateNormalImpl delegate;
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(delegate);
    sptr<NetManagerStandard::NetHandle> netHandle = nullptr;
    sptr<NetManagerStandard::NetAllCapabilities> netAllCap = nullptr;
    int32_t status = observer->NetCapabilitiesChange(netHandle, netAllCap);
    EXPECT_EQ(status, 0);

    netHandle = new (std::nothrow) NetHandle();
    status = observer->NetCapabilitiesChange(netHandle, netAllCap);
    EXPECT_EQ(status, 0);

    netHandle = nullptr;
    netAllCap = new (std::nothrow) NetAllCapabilities();
    status = observer->NetCapabilitiesChange(netHandle, netAllCap);
    EXPECT_EQ(status, 0);

    delete observer;
    delete netHandle;
    delete netAllCap;
}

/**
* @tc.name: NetCapabilitiesChange002
* @tc.desc: NetCapabilitiesChange testing different branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, NetCapabilitiesChange002, TestSize.Level1)
{
    NetworkDelegateNormalImpl delegate;
    DmDeviceInfo& info = const_cast<DmDeviceInfo &>(delegate.cloudDmInfo_);
    std::fill(info.networkId, info.networkId + sizeof(info.networkId), '\0');
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(delegate);
    sptr<NetManagerStandard::NetHandle> netHandle = new (std::nothrow) NetHandle();
    sptr<NetManagerStandard::NetAllCapabilities> netAllCap = new (std::nothrow) NetAllCapabilities();
    EXPECT_FALSE(netAllCap->netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED));
    EXPECT_FALSE(!netAllCap->bearerTypes_.empty());
    int32_t status = observer->NetCapabilitiesChange(netHandle, netAllCap);
    EXPECT_EQ(status, 0);

    netAllCap->netCaps_.insert(NetManagerStandard::NET_CAPABILITY_VALIDATED);
    EXPECT_TRUE(netAllCap->netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED));
    EXPECT_FALSE(!netAllCap->bearerTypes_.empty());
    status = observer->NetCapabilitiesChange(netHandle, netAllCap);
    EXPECT_EQ(status, 0);

    sptr<NetManagerStandard::NetAllCapabilities> netAllCaps = new (std::nothrow) NetAllCapabilities();
    netAllCaps->bearerTypes_.insert(NetManagerStandard::BEARER_WIFI);
    EXPECT_FALSE(netAllCaps->netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED));
    EXPECT_TRUE(!netAllCaps->bearerTypes_.empty());
    status = observer->NetCapabilitiesChange(netHandle, netAllCaps);
    EXPECT_EQ(status, 0);

    netAllCaps->netCaps_.insert(NetManagerStandard::NET_CAPABILITY_VALIDATED);
    EXPECT_TRUE(netAllCaps->netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED));
    EXPECT_TRUE(!netAllCaps->bearerTypes_.empty());
    status = observer->NetCapabilitiesChange(netHandle, netAllCaps);
    EXPECT_EQ(status, 0);

    delete observer;
    delete netHandle;
    delete netAllCap;
    delete netAllCaps;
}

/**
* @tc.name: Convert
* @tc.desc: Convert testing different branching scenarios.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(NetworkDelegateNormalImplTest, Convert, TestSize.Level1)
{
    NetManagerStandard::NetBearType bearType = NetManagerStandard::BEARER_WIFI;
    NetworkDelegateNormalImpl::NetworkType status = Convert(bearType);
    EXPECT_EQ(status, NetworkDelegate::WIFI);

    bearType = NetManagerStandard::BEARER_CELLULAR;
    status = Convert(bearType);
    EXPECT_EQ(status, NetworkDelegate::CELLULAR);

    bearType = NetManagerStandard::BEARER_ETHERNET;
    status = Convert(bearType);
    EXPECT_EQ(status, NetworkDelegate::ETHERNET);

    bearType = NetManagerStandard::BEARER_VPN;
    status = Convert(bearType);
    EXPECT_EQ(status, NetworkDelegate::OTHER);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
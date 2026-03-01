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

#include <fuzzer/FuzzedDataProvider.h>

#include "udmfserviceprivilege_fuzzer.h"

#include "accesstoken_kit.h"
#include "distributeddata_udmf_ipc_interface_code.h"
#include "itypes_util.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"
#include "udmf_service_impl.h"
#include "udmf_types_util.h"
#include "bootstrap.h"
#include "observer_factory.h"
#include "drag_observer.h"
#include "data_handler.h"
#include "delay_data_acquire_container.h"
#include "device_manager_adapter.h"
#include "kvstore_meta_manager.h"
#include "lifecycle_manager.h"
#include "log_print.h"
#include "nativetoken_kit.h"
#include "udmf_notifier_proxy.h"
#include "udmf_service_impl.h"

using namespace OHOS::UDMF;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS {
const std::u16string INTERFACE_TOKEN = u"OHOS.UDMF.UdmfService";
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;
static constexpr int ID_LEN = 32;
static constexpr int MINIMUM = 48;
static constexpr int MAXIMUM = 121;
static constexpr int USERID = 100;
static constexpr int INSTINDEX = 0;
static constexpr const char *BUNDLENAME = "com.test.demo";

static void GrantPermissionNative()
{
    const char *perms[3] = {
        "ohos.permission.DISTRIBUTED_DATASYNC",
        "ohos.permission.ACCESS_SERVICE_DM",
        "ohos.permission.MONITOR_DEVICE_NETWORK_STATE" // perms[2] is a permission parameter
    };
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 3,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
}

static void SetUpTestCase(void) {}
static void TearDownTestCase(void) {}
static void DeleteTestHapToken();
void SetUp() {}
void TearDown()
{
    DeleteTestHapToken();
}

void AllocTestHapToken()
{
    HapInfoParams info = {
        .userID = USERID,
        .bundleName = BUNDLENAME,
        .instIndex = INSTINDEX,
        .appIDDesc = "ohos.test.demo1"
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = BUNDLENAME,
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "test1",
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.test",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}

void DeleteTestHapToken()
{
    auto tokenId = AccessTokenKit::GetHapTokenID(USERID, BUNDLENAME, INSTINDEX);
    AccessTokenKit::DeleteToken(tokenId);
}

QueryOption GenerateFuzzQueryOption(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("drag", "com.test.demo", groupIdStr);
    QueryOption query;
    query.key = udKey.GetUnifiedKey();
    query.intention = Intention::UD_INTENTION_DRAG;
    query.tokenId = provider.ConsumeIntegral<uint32_t>();
    return query;
}

void AddPrivilegeDataFuzz(FuzzedDataProvider &provider)
{
    GrantPermissionNative();
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executor);
    udmfServiceImpl->OnBind(
        { "UdmfServicePrivilegeFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    AllocTestHapToken();
    QueryOption query = GenerateFuzzQueryOption(provider);

    Privilege privilege;
    privilege.tokenId = 1;
    privilege.readPermission = "read";
    privilege.writePermission = "write";

    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    ITypesUtil::Marshal(request, query, privilege);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::ADD_PRIVILEGE),
        request, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServicePrivilegeFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void IsReadAndKeepFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    Privilege privilege;
    privilege.tokenId = provider.ConsumeIntegral<uint32_t>();
    privilege.readPermission = "read";
    privilege.writePermission = "write";
    Privilege privilege2;
    privilege2.tokenId = provider.ConsumeIntegral<uint32_t>();
    privilege2.readPermission = "readAndKeep";
    privilege2.writePermission = "write";
    std::vector<Privilege> privileges1 = { privilege, privilege2 };
    QueryOption query = GenerateFuzzQueryOption(provider);
    udmfServiceImpl->IsReadAndKeep(privileges1, query);
    std::vector<Privilege> privileges2 = { privilege };
    udmfServiceImpl->IsReadAndKeep(privileges2, query);
}

void HasDatahubPriviledgeFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    auto bundleName = provider.ConsumeRandomLengthString();
    udmfServiceImpl->HasDatahubPriviledge(bundleName);
}

void AddPrivilegeFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    QueryOption query = GenerateFuzzQueryOption(provider);
    query.tokenId = OHOS::Security::AccessToken::AccessTokenKit::GetNativeTokenId("msdp");
    Privilege privilege;
    privilege.tokenId = 1;
    privilege.readPermission = provider.ConsumeRandomLengthString();
    privilege.writePermission = provider.ConsumeRandomLengthString();
    udmfServiceImpl->AddPrivilege(query, privilege);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Security::AccessToken::AccessTokenID tokenId =
        OHOS::Security::AccessToken::AccessTokenKit::GetHapTokenID(OHOS::USERID, OHOS::BUNDLENAME, OHOS::INSTINDEX);
    SetSelfTokenID(tokenId);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::SetUpTestCase();
    OHOS::AddPrivilegeDataFuzz(provider);
    OHOS::IsReadAndKeepFuzz(provider);
    OHOS::HasDatahubPriviledgeFuzz(provider);
    OHOS::AddPrivilegeFuzz(provider);
    OHOS::TearDownTestCase();
    return 0;
}
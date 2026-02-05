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

#include "udmfservicedelaydata_fuzzer.h"

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
static constexpr const char *SCHEME_SEPARATOR = "udmf://drag/com.example.app/sajhjh";

static void GrantPermissionNative()
{
    const char *perms[4] = {
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

std::shared_ptr<UdmfServiceImpl> InitializeEnvironment()
{
    GrantPermissionNative();
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    auto executors = std::make_shared<OHOS::ExecutorPool>(NUM_MAX, NUM_MIN);
    DmAdapter::GetInstance().Init(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    AllocTestHapToken();
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    return udmfServiceImpl;
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

void SetDelayInfoFuzz(FuzzedDataProvider &provider)
{
    GrantPermissionNative();
    DistributedData::Bootstrap::GetInstance().LoadComponents();
    DistributedData::Bootstrap::GetInstance().LoadDirectory();
    DistributedData::Bootstrap::GetInstance().LoadCheckers();
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executor);
    udmfServiceImpl->OnBind(
        { "UdmfServiceDelayDataFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    AllocTestHapToken();
    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.recordCount = provider.ConsumeIntegral<uint32_t>();
    dataLoadInfo.types.emplace(provider.ConsumeRandomLengthString());
    dataLoadInfo.sequenceKey = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(requestUpdate, dataLoadInfo);
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    ITypesUtil::Marshal(requestUpdate, iUdmfNotifier);
    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DELAY_INFO),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceDelayDataFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void SetDelayInfoImplFuzz(FuzzedDataProvider &provider)
{
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.recordCount = provider.ConsumeIntegral<uint32_t>();
    dataLoadInfo.types.emplace(provider.ConsumeRandomLengthString());
    dataLoadInfo.sequenceKey = provider.ConsumeRandomLengthString();
    std::string key = provider.ConsumeRandomLengthString();
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    udmfServiceImpl->SetDelayInfo(dataLoadInfo, iUdmfNotifier, key);
}

void PushDelayDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
    udmfServiceImpl->OnBind(
        { "UdmfServiceDelayDataFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });

    MessageParcel requestUpdate;
    requestUpdate.WriteInterfaceToken(INTERFACE_TOKEN);
    std::string key = SCHEME_SEPARATOR;
    ITypesUtil::Marshal(requestUpdate, key);
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    auto record = std::make_shared<UnifiedRecord>(FILE_URI, obj);
    data.AddRecord(record);
    ITypesUtil::Marshal(requestUpdate, data);

    MessageParcel replyUpdate;
    udmfServiceImpl->OnRemoteRequest(static_cast<uint32_t>(UdmfServiceInterfaceCode::SET_DELAY_DATA),
        requestUpdate, replyUpdate);
    udmfServiceImpl->OnBind(
        { "UdmfServiceDelayDataFuzzTest", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), nullptr });
    executor = nullptr;
}

void PushDelayDataImplFuzz(FuzzedDataProvider &provider)
{
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    std::string key = SCHEME_SEPARATOR;
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    udmfServiceImpl->PushDelayData(key, data);
}

void GetDataIfAvailableFuzz(FuzzedDataProvider &provider)
{
    std::string key = provider.ConsumeRandomLengthString();
    DataLoadInfo dataLoadInfo;
    dataLoadInfo.recordCount = provider.ConsumeIntegral<uint32_t>();
    dataLoadInfo.types.emplace(provider.ConsumeRandomLengthString());
    dataLoadInfo.sequenceKey = provider.ConsumeRandomLengthString();
    sptr<IRemoteObject> iUdmfNotifier = nullptr;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    std::shared_ptr<UnifiedData> unifiedData = std::make_shared<UnifiedData>();
    unifiedData->AddRecord(std::make_shared<UnifiedRecord>(FILE_URI, obj));
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    udmfServiceImpl->GetDataIfAvailable(key, dataLoadInfo, iUdmfNotifier, unifiedData);
}

void FillDelayUnifiedDataFuzz(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> groupId(ID_LEN, '0');
    for (size_t i = 0; i < groupId.size(); ++i) {
        groupId[i] = provider.ConsumeIntegralInRange<uint8_t>(MINIMUM, MAXIMUM);
    }
    std::string groupIdStr(groupId.begin(), groupId.end());
    UnifiedKey udKey = UnifiedKey("drag", "com.test.demo", groupIdStr);
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    data.AddRecord(std::make_shared<UnifiedRecord>(FILE_URI, obj));
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    udmfServiceImpl->FillDelayUnifiedData(udKey, data);
}

void UpdateDelayDataFuzz(FuzzedDataProvider &provider)
{
    std::string key = SCHEME_SEPARATOR + provider.ConsumeRandomLengthString();
    UnifiedData data;
    std::shared_ptr<Object> obj = std::make_shared<Object>();
    obj->value_[UNIFORM_DATA_TYPE] = "general.file-uri";
    obj->value_[FILE_URI_PARAM] = provider.ConsumeRandomLengthString();
    obj->value_[FILE_TYPE] = provider.ConsumeRandomLengthString();
    data.AddRecord(std::make_shared<UnifiedRecord>(FILE_URI, obj));
    data.runtime_ = std::make_shared<Runtime>();
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = InitializeEnvironment();
    udmfServiceImpl->UpdateDelayData(key, data);
    udmfServiceImpl->GetDevicesForDelayData();
}

void PushDelayDataToRemoteFuzz(FuzzedDataProvider &provider)
{
    QueryOption query = GenerateFuzzQueryOption(provider);
    std::vector<std::string> devices(ID_LEN, "0");
    for (size_t i = 0; i < devices.size(); ++i) {
        devices[i] = provider.ConsumeRandomLengthString();
    }
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = InitializeEnvironment();
    udmfServiceImpl->PushDelayDataToRemote(query, devices);
}

void HandleRemoteDelayDataFuzz(FuzzedDataProvider &provider)
{
    std::string key = provider.ConsumeRandomLengthString();
    std::shared_ptr<UdmfServiceImpl> udmfServiceImpl = std::make_shared<UdmfServiceImpl>();
    udmfServiceImpl->HandleRemoteDelayData(key);
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
    OHOS::SetDelayInfoFuzz(provider);
    OHOS::PushDelayDataFuzz(provider);
    OHOS::GetDataIfAvailableFuzz(provider);
    OHOS::FillDelayUnifiedDataFuzz(provider);
    OHOS::UpdateDelayDataFuzz(provider);
    OHOS::PushDelayDataToRemoteFuzz(provider);
    OHOS::HandleRemoteDelayDataFuzz(provider);
    OHOS::TearDownTestCase();
    return 0;
}
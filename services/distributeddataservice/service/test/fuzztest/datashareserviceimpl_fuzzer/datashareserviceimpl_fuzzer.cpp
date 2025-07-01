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

#include "datashareserviceimpl_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "data_share_service_impl.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"

using namespace OHOS::DataShare;
namespace OHOS {

void NotifyChangeFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string uri = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    dataShareServiceImpl->NotifyChange(uri, userId);
}

void GetCallerInfoFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string bundleName = "bundlename";
    int32_t appIndex = provider.ConsumeIntegral<int32_t>();
    dataShareServiceImpl->GetCallerInfo(bundleName, appIndex);
}

void MakeMetaDataFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string userId = provider.ConsumeRandomLengthString();
    std::string deviceId = provider.ConsumeRandomLengthString();
    std::string storeId = provider.ConsumeRandomLengthString();
    dataShareServiceImpl->MakeMetaData(bundleName, userId, deviceId, storeId);
}

void OnAppUninstallFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    dataShareServiceImpl->OnAppUninstall(bundleName, userId, index);
}

void OnAppUpdateFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    dataShareServiceImpl->OnAppUpdate(bundleName, userId, index);
}

void RegisterObserverFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string uri = provider.ConsumeRandomLengthString();
    sptr<OHOS::IRemoteObject> remoteObj = nullptr;
    dataShareServiceImpl->RegisterObserver(uri, remoteObj);
}

void UnregisterObserverFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string uri = provider.ConsumeRandomLengthString();
    sptr<OHOS::IRemoteObject> remoteObj = nullptr;
    dataShareServiceImpl->UnregisterObserver(uri, remoteObj);
}

void VerifyAcrossAccountsPermissionFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    int32_t curId = provider.ConsumeIntegral<int32_t>();
    int32_t visId = provider.ConsumeIntegral<int32_t>();
    std::string permission = provider.ConsumeRandomLengthString();
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    dataShareServiceImpl->VerifyAcrossAccountsPermission(curId, visId, permission, tokenId);
}

void CheckAllowListFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    uint32_t currentUserId = provider.ConsumeIntegral<uint32_t>();
    uint32_t callerTokenId = provider.ConsumeIntegral<uint32_t>();
    std::vector<AllowList> allowLists;
    AllowList list1;
    list1.appIdentifier = provider.ConsumeRandomLengthString();
    list1.onlyMain = provider.ConsumeBool();
    AllowList list2;
    list2.appIdentifier = provider.ConsumeRandomLengthString();
    list2.onlyMain = provider.ConsumeBool();
    allowLists.push_back(list1);
    allowLists.push_back(list2);
    dataShareServiceImpl->CheckAllowList(currentUserId, callerTokenId, allowLists);
}

void ReportExcuteFaultFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    uint32_t callingTokenId = provider.ConsumeIntegral<uint32_t>();
    int32_t errCode = provider.ConsumeIntegral<int32_t>();
    std::string func = provider.ConsumeRandomLengthString();
    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.moduleName = provider.ConsumeRandomLengthString();
    providerInfo.bundleName = provider.ConsumeRandomLengthString();
    providerInfo.storeName = provider.ConsumeRandomLengthString();
    dataShareServiceImpl->ReportExcuteFault(callingTokenId, providerInfo, errCode, func);
}

void VerifyPermissionFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    std::string permission = provider.ConsumeRandomLengthString();
    bool isFromExtension = provider.ConsumeBool();
    int32_t tokenId = provider.ConsumeIntegral<int32_t>();
    dataShareServiceImpl->VerifyPermission(bundleName, permission, isFromExtension, tokenId);
}

void OnAppExitFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    pid_t uid = provider.ConsumeIntegral<int32_t>();
    pid_t pid = provider.ConsumeIntegral<int32_t>();
    uint32_t tokenId = provider.ConsumeIntegral<uint32_t>();
    std::string bundleName = provider.ConsumeRandomLengthString();
    dataShareServiceImpl->OnAppExit(uid, pid, tokenId, bundleName);
}

void DumpDataShareServiceInfoFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    int fd = provider.ConsumeIntegral<int>();
    std::map<std::string, std::vector<std::string>> params;
    params["key1"] = {"value1", "value2"};
    params["key2"] = {"val_b1", "val_b2", "val_b3"};
    dataShareServiceImpl->DumpDataShareServiceInfo(fd, params);
}

void DataShareStaticOnClearAppStorageFuzz(FuzzedDataProvider &provider)
{
    DataShareServiceImpl::DataShareStatic dataShareStatic;
    std::string bundleName = provider.ConsumeRandomLengthString();
    int32_t userId = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    int32_t tokenId = provider.ConsumeIntegral<int32_t>();
    dataShareStatic.OnClearAppStorage(bundleName, userId, index, tokenId);
}

void TimerReceiverOnReceiveEventFuzz(FuzzedDataProvider &provider)
{
    DataShareServiceImpl::TimerReceiver tmerReceiver;
    EventFwk::Want want;
    EventFwk::CommonEventData commonEventData(want);
    commonEventData.SetWant(want);
    tmerReceiver.OnReceiveEvent(commonEventData);
}

void OnUserChangeFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(0, 10);
    std::string user = provider.ConsumeRandomLengthString();
    std::string account = provider.ConsumeRandomLengthString();
    dataShareServiceImpl->OnUserChange(code, user, account);
}

void DataShareStaticOnAppUninstall(FuzzedDataProvider &provider)
{
    DataShareServiceImpl::DataShareStatic dataShareStatic;
    std::string bundleName = provider.ConsumeRandomLengthString();
    int32_t user = provider.ConsumeIntegral<int32_t>();
    int32_t index = provider.ConsumeIntegral<int32_t>();
    dataShareStatic.OnAppUninstall(bundleName, user, index);
}

void AllowCleanDataLaunchAppFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    int32_t evtId = provider.ConsumeIntegral<int32_t>();
    DistributedData::RemoteChangeEvent::DataInfo dataInfo;
    dataInfo.userId = provider.ConsumeRandomLengthString();
    dataInfo.storeId = provider.ConsumeRandomLengthString();
    dataInfo.deviceId = provider.ConsumeRandomLengthString();
    dataInfo.bundleName = provider.ConsumeRandomLengthString();
    DistributedData::RemoteChangeEvent event(evtId, std::move(dataInfo));
    bool launchForCleanData = provider.ConsumeBool();
    dataShareServiceImpl->AllowCleanDataLaunchApp(event, launchForCleanData);
}

void AutoLaunchFuzz(FuzzedDataProvider &provider)
{
    std::shared_ptr<DataShareServiceImpl> dataShareServiceImpl = std::make_shared<DataShareServiceImpl>();
    int32_t evtId = provider.ConsumeIntegral<int32_t>();
    DistributedData::RemoteChangeEvent::DataInfo dataInfo;
    dataInfo.userId = provider.ConsumeRandomLengthString();
    dataInfo.storeId = provider.ConsumeRandomLengthString();
    dataInfo.deviceId = provider.ConsumeRandomLengthString();
    dataInfo.bundleName = provider.ConsumeRandomLengthString();
    DistributedData::RemoteChangeEvent event(evtId, std::move(dataInfo));
    dataShareServiceImpl->AutoLaunch(event);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::NotifyChangeFuzz(provider);
    OHOS::GetCallerInfoFuzz(provider);
    OHOS::MakeMetaDataFuzz(provider);
    OHOS::OnAppUninstallFuzz(provider);
    OHOS::OnAppUpdateFuzz(provider);
    OHOS::RegisterObserverFuzz(provider);
    OHOS::UnregisterObserverFuzz(provider);
    OHOS::VerifyAcrossAccountsPermissionFuzz(provider);
    OHOS::CheckAllowListFuzz(provider);
    OHOS::ReportExcuteFaultFuzz(provider);
    OHOS::VerifyPermissionFuzz(provider);
    OHOS::OnAppExitFuzz(provider);
    OHOS::DumpDataShareServiceInfoFuzz(provider);
    OHOS::DataShareStaticOnClearAppStorageFuzz(provider);
    OHOS::TimerReceiverOnReceiveEventFuzz(provider);
    OHOS::OnUserChangeFuzz(provider);
    OHOS::DataShareStaticOnAppUninstall(provider);
    OHOS::AllowCleanDataLaunchAppFuzz(provider);
    OHOS::AutoLaunchFuzz(provider);
    return 0;
}
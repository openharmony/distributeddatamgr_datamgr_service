/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cloudservicestub_fuzzer.h"

#include <cstddef>
#include <cstdint>

#include "accesstoken_kit.h"
#include "cloud_service_impl.h"
#include "ipc_skeleton.h"
#include "message_parcel.h"
#include "securec.h"
#include "token_setproc.h"

using namespace OHOS::CloudData;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
constexpr int USER_ID = 100;
constexpr int INST_INDEX = 0;
const std::u16string INTERFACE_TOKEN = u"OHOS.CloudData.CloudServer";
constexpr uint32_t CODE_MIN = 0;
constexpr uint32_t CODE_MAX = CloudService::TransId::TRANS_BUTT + 1;
constexpr size_t NUM_MIN = 5;
constexpr size_t NUM_MAX = 12;

void AllocAndSetHapToken()
{
    HapInfoParams info = {
        .userID = USER_ID,
        .bundleName = "ohos.test.demo",
        .instIndex = INST_INDEX,
        .appIDDesc = "ohos.test.demo",
        .isSystemApp = true
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {},
        .permStateList = {
            {
                .permissionName = "ohos.permission.CLOUDDATA_CONFIG",
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

bool OnRemoteRequestFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnRemoteRequest(code, request, reply);
    return true;
}

bool OnEnableCloudFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    
    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnEnableCloud(code, request, reply);
    XCollie xcollie(
        __FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY, RESTART_SERVICE_TIME_THRESHOLD);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_ENABLE_CLOUD, Fault::CSF_CLOUD_INFO, "", "EnableCloud ret=" + std::to_string(status));
        return status;
    }
    cloudInfo.enableCloud = true;
    for (const auto &[bundle, value] : switches) {
        if (!cloudInfo.Exist(bundle)) {
            continue;
        }
        cloudInfo.apps[bundle].cloudSwitch = (value == SWITCH_ON);
        ZLOGI("EnableCloud: change app[%{public}s] switch to %{public}d", bundle.c_str(), value);
    }
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, CloudSyncScene::ENABLE_CLOUD,
        { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
    ZLOGI("EnableCloud success, id:%{public}s, count:%{public}zu", Anonymous::Change(id).c_str(), switches.size());
    return true;
}

bool OnDisableCloudFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnDisableCloud(code, request, reply);
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    ReleaseUserInfo(user, CloudSyncScene::DISABLE_CLOUD);
    std::lock_guard<decltype(rwMetaMutex_)> lock(rwMetaMutex_);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_DISABLE_CLOUD, Fault::CSF_CLOUD_INFO, "", "DisableCloud ret=" + std::to_string(status));
        return status;
    }
    if (cloudInfo.id != id) {
        ZLOGE("invalid args, [input] id:%{public}s, [exist] id:%{public}s", Anonymous::Change(id).c_str(),
            Anonymous::Change(cloudInfo.id).c_str());
        return INVALID_ARGUMENT;
    }
    cloudInfo.enableCloud = false;
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    StoreMetaMapping meta(storeInfo);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        ZLOGE("failed, no store mapping bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    if (!meta.cloudPath.empty() && meta.dataDir != meta.cloudPath &&
        !MetaDataManager::GetInstance().LoadMeta(meta.GetCloudStoreMetaKey(), meta, true)) {
        ZLOGE("failed, no store meta bundleName:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    auto [status, store] = SyncManager::GetStore(meta, storeInfo.user, true);
    if (store == nullptr) {
        ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
        return { GeneralError::E_ERROR, nullptr };
    }
    return store->PreSharing(query);
    Execute(GenTask(0, cloudInfo.user, CloudSyncScene::DISABLE_CLOUD, { WORK_STOP_CLOUD_SYNC, WORK_SUB }));
    ZLOGI("DisableCloud success, id:%{public}s", Anonymous::Change(id).c_str());
    return true;
}

bool OnChangeAppSwitchFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnChangeAppSwitch(code, request, reply);
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    CloudSyncScene scene;
    if (appSwitch == SWITCH_ON) {
        scene = CloudSyncScene::SWITCH_ON;
    } else {
        scene = CloudSyncScene::SWITCH_OFF;
    }
    std::lock_guard<decltype(rwMetaMutex_)> lock(rwMetaMutex_);
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS || !cloudInfo.enableCloud) {
        Report(appSwitch == SWITCH_ON ? FT_SWITCH_ON : FT_SWITCH_OFF, Fault::CSF_CLOUD_INFO, bundleName,
            "ChangeAppSwitch ret = " + std::to_string(status));
        return status;
    }
    if (cloudInfo.id != id) {
        ZLOGW("invalid args, [input] id:%{public}s, [exist] id:%{public}s, bundleName:%{public}s",
            Anonymous::Change(id).c_str(), Anonymous::Change(cloudInfo.id).c_str(), bundleName.c_str());
        Execute(GenTask(0, cloudInfo.user, scene, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB,
            WORK_DO_CLOUD_SYNC }));
        return INVALID_ARGUMENT;
    }
    if (!cloudInfo.Exist(bundleName)) {
        std::tie(status, cloudInfo) = GetCloudInfoFromServer(user);
        if (status != SUCCESS || !cloudInfo.enableCloud || cloudInfo.id != id || !cloudInfo.Exist(bundleName)) {
            ZLOGE("invalid args, status:%{public}d, enableCloud:%{public}d, [input] id:%{public}s,"
                  "[exist] id:%{public}s, bundleName:%{public}s", status, cloudInfo.enableCloud,
                  Anonymous::Change(id).c_str(), Anonymous::Change(cloudInfo.id).c_str(), bundleName.c_str());
            Report(appSwitch == SWITCH_ON ? FT_SWITCH_ON : FT_SWITCH_OFF, Fault::CSF_CLOUD_INFO, bundleName,
                "ChangeAppSwitch ret=" + std::to_string(status));
            return INVALID_ARGUMENT;
        }
        ZLOGI("add app switch, bundleName:%{public}s", bundleName.c_str());
    }
    cloudInfo.apps[bundleName].cloudSwitch = (appSwitch == SWITCH_ON);
    if (!MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true)) {
        return ERROR;
    }
    Execute(GenTask(0, cloudInfo.user, scene, { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_SUB }));
    if (cloudInfo.enableCloud && appSwitch == SWITCH_ON) {
        SyncManager::SyncInfo info(cloudInfo.user, bundleName);
        syncManager_.DoCloudSync(info);
    }
    ZLOGI("ChangeAppSwitch success, id:%{public}s app:%{public}s, switch:%{public}d", Anonymous::Change(id).c_str(),
        bundleName.c_str(), appSwitch);
    return true;
}

bool OnNotifyDataChangeFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnNotifyDataChange(code, request, reply);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto user = Account::GetInstance()->GetUserByToken(tokenId);
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (CheckNotifyConditions(id, bundleName, cloudInfo) != E_OK) {
        return INVALID_ARGUMENT;
    }
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    ExtraData exData;
    if (eventId != DATA_CHANGE_EVENT_ID || extraData.empty() || !exData.Unmarshall(extraData)) {
        ZLOGE("invalid args, eventId:%{public}s, user:%{public}d, extraData is %{public}s", eventId.c_str(),
            userId, extraData.empty() ? "empty" : "not empty");
        return INVALID_ARGUMENT;
    }
    std::vector<int32_t> users;
    if (userId != INVALID_USER_ID) {
        users.emplace_back(userId);
    } else {
        Account::GetInstance()->QueryForegroundUsers(users);
    }
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        auto &bundleName = exData.info.bundleName;
        auto &prepareTraceId = exData.info.context.prepareTraceId;
        auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
        if (CheckNotifyConditions(exData.info.accountId, bundleName, cloudInfo) != E_OK) {
            ZLOGD("invalid user:%{public}d, traceId:%{public}s", user, prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        auto schemaKey = CloudInfo::GetSchemaKey(user, bundleName);
        SchemaMeta schemaMeta;
        if (!MetaDataManager::GetInstance().LoadMeta(schemaKey, schemaMeta, true)) {
            ZLOGE("no exist meta, user:%{public}d, traceId:%{public}s", user, prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        auto dbInfos = GetDbInfoFromExtraData(exData, schemaMeta);
        if (dbInfos.empty()) {
            ZLOGE("GetDbInfoFromExtraData failed, empty database info. traceId:%{public}s.", prepareTraceId.c_str());
            syncManager_.Report({ user, bundleName, prepareTraceId, SyncStage::END, INVALID_ARGUMENT });
            return INVALID_ARGUMENT;
        }
        for (const auto &dbInfo : dbInfos) {
            syncManager_.DoCloudSync(SyncManager::SyncInfo(
                { cloudInfo.user, bundleName, dbInfo.first, dbInfo.second, MODE_PUSH, prepareTraceId }));
        }
    }
    auto [status, cloudInfo] = GetCloudInfoFromServer(user);
    if (status != SUCCESS) {
        ZLOGE("user:%{public}d, status:%{public}d", user, status);
        Report(GetDfxFaultType(scene), Fault::CSF_CLOUD_INFO, "", "UpdateCloudInfo ret=" + std::to_string(status));
        return false;
    }
    ZLOGI("[server] id:%{public}s, enableCloud:%{public}d, user:%{public}d, app size:%{public}zu",
          Anonymous::Change(cloudInfo.id).c_str(), cloudInfo.enableCloud, cloudInfo.user, cloudInfo.apps.size());
    CloudInfo oldInfo;
    if (!MetaDataManager::GetInstance().LoadMeta(cloudInfo.GetKey(), oldInfo, true)) {
        MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
        return true;
    }
    MetaDataManager::GetInstance().SaveMeta(cloudInfo.GetKey(), cloudInfo, true);
    if (oldInfo.id != cloudInfo.id) {
        ReleaseUserInfo(user, scene);
        ZLOGE("different id, [server] id:%{public}s, [meta] id:%{public}s", Anonymous::Change(cloudInfo.id).c_str(),
            Anonymous::Change(oldInfo.id).c_str());
        MetaDataManager::GetInstance().DelMeta(Subscription::GetKey(user), true);
        std::map<std::string, int32_t> actions;
        for (auto &[bundle, app] : cloudInfo.apps) {
            MetaDataManager::GetInstance().DelMeta(Subscription::GetRelationKey(user, bundle), true);
            actions[bundle] = GeneralStore::CleanMode::CLOUD_INFO;
        }
        DoClean(oldInfo, actions);
        Report(GetDfxFaultType(scene), Fault::CSF_CLOUD_INFO, "",
            "Clean: different id, new:" + Anonymous::Change(cloudInfo.id) + ", old:" + Anonymous::Change(oldInfo.id));
    }
    return true;
}

bool OnQueryLastSyncInfoFuzz(FuzzedDataProvider &provider)
{
    static std::shared_ptr<CloudServiceImpl> cloudServiceImpl = std::make_shared<CloudServiceImpl>();
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        cloudServiceImpl->OnBind(
            { "CloudServiceStubFuzz", static_cast<uint32_t>(IPCSkeleton::GetSelfTokenID()), std::move(executor) });
        AllocAndSetHapToken();
    });

    uint32_t code = provider.ConsumeIntegralInRange<uint32_t>(CODE_MIN, CODE_MAX);
    std::vector<uint8_t> remainingData = provider.ConsumeRemainingBytes<uint8_t>();
    MessageParcel request;
    request.WriteInterfaceToken(INTERFACE_TOKEN);
    request.WriteBuffer(static_cast<void *>(remainingData.data()), remainingData.size());
    request.RewindRead(0);
    MessageParcel reply;
    std::shared_ptr<CloudServiceStub> cloudServiceStub = cloudServiceImpl;
    cloudServiceStub->OnQueryLastSyncInfo(code, request, reply);
    QueryLastResults results;
    auto user = Account::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    auto [status, cloudInfo] = GetCloudInfo(user);
    if (status != SUCCESS) {
        Report(FT_QUERY_INFO, Fault::CSF_CLOUD_INFO, bundleName,
            "QueryLastSyncInfo ret=" + std::to_string(status) + ",storeId=" + storeId);
        return { ERROR, results };
    }
    if (cloudInfo.apps.find(bundleName) == cloudInfo.apps.end()) {
        ZLOGE("Invalid bundleName: %{public}s", bundleName.c_str());
        return { INVALID_ARGUMENT, results };
    }
    std::vector<SchemaMeta> schemas;
    auto key = cloudInfo.GetSchemaPrefix(bundleName);
    if (!MetaDataManager::GetInstance().LoadMeta(key, schemas, true) || schemas.empty()) {
        return { ERROR, results };
    }

    std::vector<QueryKey> queryKeys;
    std::vector<Database> databases;
    for (const auto &schema : schemas) {
        if (schema.bundleName != bundleName) {
            continue;
        }
        databases = schema.databases;
        for (const auto &database : schema.databases) {
            if (storeId.empty() || database.alias == storeId) {
                queryKeys.push_back({ user, id, bundleName, database.name });
            }
        }
        if (queryKeys.empty()) {
            ZLOGE("Invalid storeId: %{public}s", Anonymous::Change(storeId).c_str());
            return { INVALID_ARGUMENT, results };
        }
    }
    XCollie xcollie(__FUNCTION__, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    DistributedDB::RuntimeConfig::SetCloudTranslate(std::make_shared<RdbCloudDataTranslate>());
    std::vector<int> users;
    Account::GetInstance()->QueryUsers(users);
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        CleanWaterVersion(user);
    }
    Execute(GenTask(0, 0, CloudSyncScene::SERVICE_INIT,
        { WORK_CLOUD_INFO_UPDATE, WORK_SCHEMA_UPDATE, WORK_DO_CLOUD_SYNC, WORK_SUB }));
    for (auto user : users) {
        if (user == DEFAULT_USER) {
            continue;
        }
        Subscription sub;
        sub.userId = user;
        if (!MetaDataManager::GetInstance().LoadMeta(sub.GetKey(), sub, true)) {
            continue;
        }
        InitSubTask(sub);
    }
    auto [status, cloudInfo] = GetCloudInfoFromMeta(user);
    if (status != SUCCESS) {
        return false;
    }
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    auto staticStores = CheckerManager::GetInstance().GetStaticStores();
    stores.insert(stores.end(), staticStores.begin(), staticStores.end());
    auto keys = cloudInfo.GetSchemaKey();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    OHOS::OnRemoteRequestFuzz(provider);
    OHOS::OnEnableCloudFuzz(provider);
    OHOS::OnDisableCloudFuzz(provider);
    OHOS::OnChangeAppSwitchFuzz(provider);
    OHOS::OnNotifyDataChangeFuzz(provider);
    OHOS::OnQueryLastSyncInfoFuzz(provider);    
    return 0;
}
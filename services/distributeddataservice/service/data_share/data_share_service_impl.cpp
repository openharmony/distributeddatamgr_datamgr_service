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

#define LOG_TAG "DataShareServiceImpl"

#include "data_share_service_impl.h"

#include <cstdint>
#include <utility>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "app_connect_manager.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "data_ability_observer_interface.h"
#include "dataobs_mgr_client.h"
#include "datashare_errno.h"
#include "datashare_template.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "extension_ability_manager.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "matching_skills.h"
#include "permit_delegate.h"
#include "rdb_helper.h"
#include "scheduler_manager.h"
#include "subscriber_managers/published_data_subscriber_manager.h"
#include "template_data.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using FeatureSystem = DistributedData::FeatureSystem;
using DumpManager = OHOS::DistributedData::DumpManager;
using ProviderInfo = DataProviderConfig::ProviderInfo;
using namespace OHOS::DistributedData;
__attribute__((used)) DataShareServiceImpl::Factory DataShareServiceImpl::factory_;
DataShareServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("data_share", []() {
        return std::make_shared<DataShareServiceImpl>();
    });
    staticActs_ = std::make_shared<DataShareStatic>();
    FeatureSystem::GetInstance().RegisterStaticActs("data_share", staticActs_);
}

DataShareServiceImpl::Factory::~Factory() {}

int32_t DataShareServiceImpl::Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("Insert enter.");
    if (!IsSilentProxyEnable(uri)) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    auto callBack = [&uri, &valuesBucket, this](ProviderInfo &providerInfo,
            DistributedData::StoreMetaData &metaData, std::shared_ptr<DBDelegate> dbDelegate) -> int32_t {
        auto ret = dbDelegate->Insert(providerInfo.tableName, valuesBucket);
        if (ret > 0) {
            NotifyChange(uri);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.currentUserId, metaData);
        }
        return ret;
    };
    return Execute(uri, IPCSkeleton::GetCallingTokenID(), false, callBack);
}

bool DataShareServiceImpl::NotifyChange(const std::string &uri)
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        ZLOGE("obsMgrClient is nullptr");
        return false;
    }

    ErrCode ret = obsMgrClient->NotifyChange(Uri(uri));
    if (ret != ERR_OK) {
        ZLOGE("obsMgrClient->NotifyChange error return %{public}d", ret);
        return false;
    }
    return true;
}

int32_t DataShareServiceImpl::Update(const std::string &uri, const DataSharePredicates &predicate,
    const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("Update enter.");
    if (!IsSilentProxyEnable(uri)) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    auto callBack = [&uri, &predicate, &valuesBucket, this](ProviderInfo &providerInfo,
            DistributedData::StoreMetaData &metaData, std::shared_ptr<DBDelegate> dbDelegate) -> int32_t {
        auto ret = dbDelegate->Update(providerInfo.tableName, predicate, valuesBucket);
        if (ret > 0) {
            NotifyChange(uri);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.currentUserId, metaData);
        }
        return ret;
    };
    return Execute(uri, IPCSkeleton::GetCallingTokenID(), false, callBack);
}

int32_t DataShareServiceImpl::Delete(const std::string &uri, const DataSharePredicates &predicate)
{
    if (!IsSilentProxyEnable(uri)) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    auto callBack = [&uri, &predicate, this](ProviderInfo &providerInfo,
            DistributedData::StoreMetaData &metaData, std::shared_ptr<DBDelegate> dbDelegate) -> int32_t {
        auto ret = dbDelegate->Delete(providerInfo.tableName, predicate);
        if (ret > 0) {
            NotifyChange(uri);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.currentUserId, metaData);
        }
        return ret;
    };
    return Execute(uri, IPCSkeleton::GetCallingTokenID(), false, callBack);
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(const std::string &uri,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns, int &errCode)
{
    ZLOGD("Query enter.");
    if (!IsSilentProxyEnable(uri)) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return nullptr;
    }
    std::shared_ptr<DataShareResultSet> resultSet;
    auto callingPid = IPCSkeleton::GetCallingPid();
    auto callBack = [&uri, &predicates, &columns, &resultSet, &callingPid](ProviderInfo &providerInfo,
            DistributedData::StoreMetaData &, std::shared_ptr<DBDelegate> dbDelegate) -> int32_t {
        auto [err, result] = dbDelegate->Query(providerInfo.tableName,
            predicates, columns, callingPid);
        resultSet = std::move(result);
        return err;
    };
    errCode = Execute(uri, IPCSkeleton::GetCallingTokenID(), true, callBack);
    return resultSet;
}

int32_t DataShareServiceImpl::AddTemplate(const std::string &uri, const int64_t subscriberId, const Template &tplt)
{
    auto context = std::make_shared<Context>(uri);
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    ZLOGI("Add template, uri %{private}s, subscriberId %{public}" PRIi64 ", bundleName %{public}s,"
          "predicates size %{public}zu.",
        uri.c_str(), subscriberId, tpltId.bundleName_.c_str(), tplt.predicates_.size());
    return templateStrategy_.Execute(context, [&uri, &tpltId, &tplt, &context]() -> int32_t {
        auto result = TemplateManager::GetInstance().Add(
            Key(uri, tpltId.subscriberId_, tpltId.bundleName_), context->currentUserId, tplt);
        RdbSubscriberManager::GetInstance().Emit(context->uri, tpltId.subscriberId_, context);
        return result;
    });
}

int32_t DataShareServiceImpl::DelTemplate(const std::string &uri, const int64_t subscriberId)
{
    auto context = std::make_shared<Context>(uri);
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    ZLOGI("Delete template, uri %{private}s, subscriberId %{public}" PRIi64 ", bundleName %{public}s.",
        DistributedData::Anonymous::Change(uri).c_str(), subscriberId, tpltId.bundleName_.c_str());
    return templateStrategy_.Execute(context, [&uri, &tpltId, &context]() -> int32_t {
        return TemplateManager::GetInstance().Delete(
            Key(uri, tpltId.subscriberId_, tpltId.bundleName_), context->currentUserId);
    });
}

bool DataShareServiceImpl::GetCallerBundleName(std::string &bundleName)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto type = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (type == Security::AccessToken::TOKEN_NATIVE) {
        return true;
    }
    if (type != Security::AccessToken::TOKEN_HAP) {
        return false;
    }
    Security::AccessToken::HapTokenInfo tokenInfo;
    auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result != Security::AccessToken::RET_SUCCESS) {
        ZLOGE("token:0x%{public}x, result:%{public}d", tokenId, result);
        return false;
    }
    bundleName = tokenInfo.bundleName;
    return true;
}

std::vector<OperationResult> DataShareServiceImpl::Publish(const Data &data, const std::string &bundleNameOfProvider)
{
    std::vector<OperationResult> results;
    std::vector<PublishedDataKey> publishedData;
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return results;
    }
    int32_t userId = -1;
    PublishedData::ClearAging();
    for (const auto &item : data.datas_) {
        auto context = std::make_shared<Context>(item.key_);
        context->version = data.version_;
        context->callerBundleName = callerBundleName;
        context->calledBundleName = bundleNameOfProvider;
        int32_t result = publishStrategy_.Execute(context, item);
        results.emplace_back(item.key_, result);
        if (result != E_OK) {
            ZLOGE("publish error, key is %{public}s", DistributedData::Anonymous::Change(item.key_).c_str());
            continue;
        }
        publishedData.emplace_back(context->uri, context->calledBundleName, item.subscriberId_);
        userId = context->currentUserId;
    }
    if (!publishedData.empty()) {
        PublishedDataSubscriberManager::GetInstance().Emit(publishedData, userId, callerBundleName);
        PublishedDataSubscriberManager::GetInstance().SetObserversNotifiedOnEnabled(publishedData);
    }
    return results;
}

Data DataShareServiceImpl::GetData(const std::string &bundleNameOfProvider, int &errorCode)
{
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return Data();
    }
    auto context = std::make_shared<Context>();
    context->callerBundleName = callerBundleName;
    context->calledBundleName = bundleNameOfProvider;
    return getDataStrategy_.Execute(context, errorCode);
}

std::vector<OperationResult> DataShareServiceImpl::SubscribeRdbData(
    const std::vector<std::string> &uris, const TemplateId &id, const sptr<IDataProxyRdbObserver> observer)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&id, &observer, &context, this]() {
            return RdbSubscriberManager::GetInstance().Add(
                Key(context->uri, id.subscriberId_, id.bundleName_), observer, context, binderInfo_.executors);
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::UnsubscribeRdbData(
    const std::vector<std::string> &uris, const TemplateId &id)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&id, &context]() {
            return RdbSubscriberManager::GetInstance().Delete(
                Key(context->uri, id.subscriberId_, id.bundleName_), context->callerTokenId);
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::EnableRdbSubs(
    const std::vector<std::string> &uris, const TemplateId &id)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&id, &context]() {
            return RdbSubscriberManager::GetInstance().Enable(
                Key(context->uri, id.subscriberId_, id.bundleName_), context);
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::DisableRdbSubs(
    const std::vector<std::string> &uris, const TemplateId &id)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&id, &context]() {
            return RdbSubscriberManager::GetInstance().Disable(
                Key(context->uri, id.subscriberId_, id.bundleName_), context->callerTokenId);
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::SubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return results;
    }
    std::vector<PublishedDataKey> publishedKeys;
    int32_t result;
    int32_t userId;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        result = subscribeStrategy_.Execute(context, [&subscriberId, &observer, &context]() {
            return PublishedDataSubscriberManager::GetInstance().Add(
                PublishedDataKey(context->uri, context->callerBundleName, subscriberId), observer,
                context->callerTokenId);
        });
        results.emplace_back(uri, result);
        if (result == E_OK) {
            publishedKeys.emplace_back(context->uri, context->callerBundleName, subscriberId);
            if (binderInfo_.executors != nullptr) {
                binderInfo_.executors->Execute([context, subscriberId]() {
                    PublishedData::UpdateTimestamp(
                        context->uri, context->calledBundleName, subscriberId, context->currentUserId);
                });
            }
            userId = context->currentUserId;
        }
    }
    if (!publishedKeys.empty()) {
        PublishedDataSubscriberManager::GetInstance().Emit(publishedKeys, userId, callerBundleName, observer);
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::UnsubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return results;
    }
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&subscriberId, &context, this]() {
            auto result = PublishedDataSubscriberManager::GetInstance().Delete(
                PublishedDataKey(context->uri, context->callerBundleName, subscriberId), context->callerTokenId);
            if (result == E_OK && binderInfo_.executors != nullptr) {
                binderInfo_.executors->Execute([context, subscriberId]() {
                    PublishedData::UpdateTimestamp(
                        context->uri, context->calledBundleName, subscriberId, context->currentUserId);
                });
            }
            return result;
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::EnablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return results;
    }
    std::vector<PublishedDataKey> publishedKeys;
    int32_t result;
    int32_t userId = -1;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        result = subscribeStrategy_.Execute(context, [&subscriberId, &context]() {
            return PublishedDataSubscriberManager::GetInstance().Enable(
                PublishedDataKey(context->uri, context->callerBundleName, subscriberId), context->callerTokenId);
        });
        if (result == E_OK && binderInfo_.executors != nullptr) {
            binderInfo_.executors->Execute([context, subscriberId]() {
                PublishedData::UpdateTimestamp(
                    context->uri, context->calledBundleName, subscriberId, context->currentUserId);
            });
        }
        results.emplace_back(uri, result);
        if (result == E_OK) {
            PublishedDataKey pKey(context->uri, context->callerBundleName, subscriberId);
            if (PublishedDataSubscriberManager::GetInstance().IsNotifyOnEnabled(pKey, context->callerTokenId)) {
                publishedKeys.emplace_back(pKey);
            }
            userId = context->currentUserId;
        }
    }
    if (!publishedKeys.empty()) {
        PublishedDataSubscriberManager::GetInstance().Emit(publishedKeys, userId, callerBundleName);
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::DisablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return results;
    }
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        results.emplace_back(uri, subscribeStrategy_.Execute(context, [&subscriberId, &context, this]() {
            auto result =  PublishedDataSubscriberManager::GetInstance().Disable(
                PublishedDataKey(context->uri, context->callerBundleName, subscriberId), context->callerTokenId);
            if (result == E_OK && binderInfo_.executors != nullptr) {
                binderInfo_.executors->Execute([context, subscriberId]() {
                    PublishedData::UpdateTimestamp(
                        context->uri, context->calledBundleName, subscriberId, context->currentUserId);
                });
            }
            return result;
        }));
    }
    return results;
}

enum DataShareKvStoreType : int32_t {
    DATA_SHARE_SINGLE_VERSION = 0,
    DISTRIBUTED_TYPE_BUTT
};

int32_t DataShareServiceImpl::OnBind(const BindInfo &binderInfo)
{
    binderInfo_ = binderInfo;
    const std::string accountId = DistributedKv::AccountDelegate::GetInstance()->GetCurrentAccountId();
    const auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(binderInfo.selfTokenId);
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "default";
    saveMeta.storeId = "data_share_data_";
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName =  binderInfo.selfName;
    saveMeta.appId = binderInfo.selfName;
    saveMeta.user = std::to_string(userId);
    saveMeta.account = accountId;
    saveMeta.tokenId = binderInfo.selfTokenId;
    saveMeta.securityLevel = DistributedKv::SecurityLevel::S1;
    saveMeta.area = DistributedKv::Area::EL1;
    saveMeta.uid = IPCSkeleton::GetCallingUid();
    saveMeta.storeType = DATA_SHARE_SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    KvDBDelegate::GetInstance(false, saveMeta.dataDir, binderInfo.executors);
    SchedulerManager::GetInstance().SetExecutorPool(binderInfo.executors);
    ExtensionAbilityManager::GetInstance().SetExecutorPool(binderInfo.executors);
    SubscribeTimeChanged();
    return E_OK;
}

void DataShareServiceImpl::OnConnectDone()
{
    std::string callerBundleName;
    if (!GetCallerBundleName(callerBundleName)) {
        ZLOGE("get bundleName error, %{public}s", callerBundleName.c_str());
        return;
    }
    AppConnectManager::Notify(callerBundleName);
}

int32_t DataShareServiceImpl::DataShareStatic::OnAppUninstall(const std::string &bundleName, int32_t user,
    int32_t index)
{
    ZLOGI("%{public}s uninstalled", bundleName.c_str());
    PublishedData::Delete(bundleName, user);
    PublishedData::ClearAging();
    TemplateData::Delete(bundleName, user);
    NativeRdb::RdbHelper::ClearCache();
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    ZLOGI("AppExit uid=%{public}d, pid=%{public}d, tokenId=0x%{public}x, bundleName=%{public}s",
        uid, pid, tokenId, bundleName.c_str());
    RdbSubscriberManager::GetInstance().Delete(tokenId);
    PublishedDataSubscriberManager::GetInstance().Delete(tokenId);
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("AppUninstall user=%{public}d, index=%{public}d, bundleName=%{public}s",
        user, index, bundleName.c_str());
    BundleMgrProxy::GetInstance()->Delete(bundleName, user);
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("AppUpdate user=%{public}d, index=%{public}d, bundleName=%{public}s",
        user, index, bundleName.c_str());
    BundleMgrProxy::GetInstance()->Delete(bundleName, user);
    return E_OK;
}

void DataShareServiceImpl::NotifyObserver(const std::string &uri)
{
    ZLOGD("%{private}s try notified", uri.c_str());
    auto context = std::make_shared<Context>(uri);
    if (!GetCallerBundleName(context->callerBundleName)) {
        ZLOGE("get bundleName error, %{private}s", uri.c_str());
        return;
    }
    auto ret = rdbNotifyStrategy_.Execute(context);
    if (ret) {
        ZLOGI("%{private}s start notified", uri.c_str());
        RdbSubscriberManager::GetInstance().Emit(uri, context);
    }
}

bool DataShareServiceImpl::SubscribeTimeChanged()
{
    ZLOGD("start");
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_TIMEZONE_CHANGED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    timerReceiver_ = std::make_shared<TimerReceiver>(subscribeInfo);
    auto result = EventFwk::CommonEventManager::SubscribeCommonEvent(timerReceiver_);
    if (!result) {
        ZLOGE("SubscribeCommonEvent err");
    }
    return result;
}

void DataShareServiceImpl::TimerReceiver::OnReceiveEvent(const EventFwk::CommonEventData &eventData)
{
    AAFwk::Want want = eventData.GetWant();
    std::string action = want.GetAction();
    ZLOGI("action:%{public}s.", action.c_str());
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_TIME_CHANGED
        || action == EventFwk::CommonEventSupport::COMMON_EVENT_TIMEZONE_CHANGED) {
        SchedulerManager::GetInstance().ReExecuteAll();
    }
}

DataShareServiceImpl::TimerReceiver::TimerReceiver(const EventFwk::CommonEventSubscribeInfo &subscriberInfo)
    : CommonEventSubscriber(subscriberInfo)
{
}

void DataShareServiceImpl::RegisterDataShareServiceInfo()
{
    DumpManager::Config serviceInfoConfig;
    serviceInfoConfig.fullCmd = "--feature-info";
    serviceInfoConfig.abbrCmd = "-f";
    serviceInfoConfig.dumpName = "FEATURE_INFO";
    serviceInfoConfig.dumpCaption = { "| Display all the service statistics" };
    DumpManager::GetInstance().AddConfig("FEATURE_INFO", serviceInfoConfig);
}

void DataShareServiceImpl::RegisterHandler()
{
    Handler handler =
        std::bind(&DataShareServiceImpl::DumpDataShareServiceInfo, this, std::placeholders::_1, std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("FEATURE_INFO", uintptr_t(this), handler);
}

void DataShareServiceImpl::DumpDataShareServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    (void)params;
    std::string info;
    dprintf(fd, "-------------------------------------DataShareServiceInfo------------------------------\n%s\n",
        info.c_str());
}

int32_t DataShareServiceImpl::OnInitialize()
{
    RegisterDataShareServiceInfo();
    RegisterHandler();
    return 0;
}

DataShareServiceImpl::~DataShareServiceImpl()
{
    DumpManager::GetInstance().RemoveHandler("FEATURE_INFO", uintptr_t(this));
}

int32_t DataShareServiceImpl::EnableSilentProxy(const std::string &uri, bool enable)
{
    uint32_t callerTokenId = IPCSkeleton::GetCallingTokenID();
    bool ret = dataShareSilentConfig_.EnableSilentProxy(callerTokenId, uri, enable);
    if (!ret) {
        ZLOGE("Enable silent proxy err, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    return E_OK;
}

bool DataShareServiceImpl::IsSilentProxyEnable(const std::string &uri)
{
    uint32_t callerTokenId = IPCSkeleton::GetCallingTokenID();
    int32_t currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        return true;
    }
    std::string calledBundleName = uriInfo.bundleName;
    uint32_t calledTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(currentUserId, calledBundleName, 0);
    if (calledTokenId == 0) {
        calledTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(0, calledBundleName, 0);
    }
    auto success = dataShareSilentConfig_.IsSilentProxyEnable(calledTokenId, currentUserId, calledBundleName, uri);
    if (!success) {
        ZLOGW("silent proxy disable, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
    }
    return success;
}

int32_t DataShareServiceImpl::RegisterObserver(const std::string &uri,
    const sptr<OHOS::IRemoteObject> &remoteObj)
{
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    DataProviderConfig providerConfig(uri, callerTokenId);
    auto [errCode, providerInfo] = providerConfig.GetProviderInfo();
    if (errCode != E_OK) {
        ZLOGE("ProviderInfo failed! token:0x%{public}x,ret:%{public}d,uri:%{public}s", callerTokenId,
            errCode, URIUtils::Anonymous(providerInfo.uri).c_str());
    }
    if (!providerInfo.allowEmptyPermission && providerInfo.readPermission.empty()) {
        ZLOGE("reject permission, tokenId:0x%{public}x, uri:%{public}s",
            callerTokenId, URIUtils::Anonymous(uri).c_str());
    }
    if (!providerInfo.readPermission.empty() &&
        !PermitDelegate::VerifyPermission(providerInfo.readPermission, callerTokenId)) {
        ZLOGE("Permission denied! token:0x%{public}x, permission:%{public}s, uri:%{public}s",
            callerTokenId, providerInfo.readPermission.c_str(),
            URIUtils::Anonymous(providerInfo.uri).c_str());
    }
    auto obServer = iface_cast<AAFwk::IDataAbilityObserver>(remoteObj);
    if (obServer == nullptr) {
        ZLOGE("ObServer is nullptr, uri: %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERR_INVALID_VALUE;
    }
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        return ERROR;
    }
    return obsMgrClient->RegisterObserver(Uri(uri), obServer);
}

int32_t DataShareServiceImpl::UnregisterObserver(const std::string &uri,
    const sptr<OHOS::IRemoteObject> &remoteObj)
{
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    DataProviderConfig providerConfig(uri, callerTokenId);
    auto [errCode, providerInfo] = providerConfig.GetProviderInfo();
    if (errCode != E_OK) {
        ZLOGE("ProviderInfo failed! token:0x%{public}x,ret:%{public}d,uri:%{public}s", callerTokenId,
            errCode, URIUtils::Anonymous(providerInfo.uri).c_str());
    }
    if (!providerInfo.allowEmptyPermission && providerInfo.readPermission.empty()) {
        ZLOGE("reject permission, tokenId:0x%{public}x, uri:%{public}s",
            callerTokenId, URIUtils::Anonymous(uri).c_str());
    }
    if (!providerInfo.readPermission.empty() &&
        !PermitDelegate::VerifyPermission(providerInfo.readPermission, callerTokenId)) {
        ZLOGE("Permission denied! token:0x%{public}x, permission:%{public}s, uri:%{public}s",
            callerTokenId, providerInfo.readPermission.c_str(),
            URIUtils::Anonymous(providerInfo.uri).c_str());
    }
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        return ERROR;
    }
    auto obServer = iface_cast<AAFwk::IDataAbilityObserver>(remoteObj);
    if (obServer == nullptr) {
        ZLOGE("ObServer is nullptr, uri: %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERR_INVALID_VALUE;
    }
    return obsMgrClient->UnregisterObserver(Uri(uri), obServer);
}

int32_t DataShareServiceImpl::Execute(const std::string &uri, const int32_t tokenId,
    bool isRead, ExecuteCallback callback)
{
    DataProviderConfig providerConfig(uri, tokenId);
    auto [errCode, provider] = providerConfig.GetProviderInfo();
    if (errCode != E_OK) {
        ZLOGE("Provider failed! token:0x%{public}x,ret:%{public}d,uri:%{public}s", tokenId,
            errCode, URIUtils::Anonymous(provider.uri).c_str());
        return errCode;
    }
    std::string permission = isRead ? provider.readPermission : provider.writePermission;
    if (!permission.empty() && !PermitDelegate::VerifyPermission(permission, tokenId)) {
        ZLOGE("Permission denied! token:0x%{public}x, permission:%{public}s, uri:%{public}s",
            tokenId, permission.c_str(), URIUtils::Anonymous(provider.uri).c_str());
        return ERROR_PERMISSION_DENIED;
    }
    DataShareDbConfig dbConfig;
    auto [code, metaData, dbDelegate] = dbConfig.GetDbConfig(provider.uri,
        provider.hasExtension, provider.bundleName, provider.storeName,
        provider.singleton ? 0 : provider.currentUserId);
    if (code != E_OK) {
        ZLOGE("Get dbConfig fail,bundleName:%{public}s,tableName:%{public}s,tokenId:0x%{public}x, uri:%{public}s",
            provider.bundleName.c_str(), provider.tableName.c_str(), tokenId,
            URIUtils::Anonymous(provider.uri).c_str());
        return code;
    }
    return callback(provider, metaData, dbDelegate);
}
} // namespace OHOS::DataShare

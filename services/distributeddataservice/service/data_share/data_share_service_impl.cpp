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

#include "account/account_delegate.h"
#include "app_connect_manager.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "concurrent_task_client.h"
#include "config_factory.h"
#include "data_ability_observer_interface.h"
#include "data_share_profile_config.h"
#include "dataobs_mgr_client.h"
#include "datashare_errno.h"
#include "datashare_radar_reporter.h"
#include "device_manager_adapter.h"
#include "datashare_template.h"
#include "directory/directory_manager.h"
#include "eventcenter/event_center.h"
#include "extension_connect_adaptor.h"
#include "dump/dump_manager.h"
#include "extension_ability_manager.h"
#include "hap_token_info.h"
#include "hiview_adapter.h"
#include "if_system_ability_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "common/utils.h"
#include "metadata/auto_launch_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "matching_skills.h"
#include "permit_delegate.h"
#include "rdb_helper.h"
#include "scheduler_manager.h"
#include "subscriber_managers/published_data_subscriber_manager.h"
#include "sys_event_subscriber.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "task_executor.h"
#include "template_data.h"
#include "utils/anonymous.h"
#include "xcollie.h"
#include "log_debug.h"
#include "dataproxy_handle_common.h"
#include "proxy_data_manager.h"
#include "datashare_observer.h"
#include "subscriber_managers/proxy_data_subscriber_manager.h"

namespace OHOS::DataShare {
using FeatureSystem = DistributedData::FeatureSystem;
using DumpManager = OHOS::DistributedData::DumpManager;
using ProviderInfo = DataProviderConfig::ProviderInfo;
using namespace OHOS::DistributedData;
__attribute__((used)) DataShareServiceImpl::Factory DataShareServiceImpl::factory_;
// decimal base
static constexpr int DECIMAL_BASE = 10;
DataShareServiceImpl::BindInfo DataShareServiceImpl::binderInfo_;
class DataShareServiceImpl::SystemAbilityStatusChangeListener
    : public SystemAbilityStatusChangeStub {
public:
    SystemAbilityStatusChangeListener()
    {
    }
    ~SystemAbilityStatusChangeListener() = default;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override
    {
    }
};

void DataShareServiceImpl::SystemAbilityStatusChangeListener::OnAddSystemAbility(
    int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("saId:%{public}d", systemAbilityId);
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        InitSubEvent();
    } else if (systemAbilityId == CONCURRENT_TASK_SERVICE_ID) {
        std::unordered_map<std::string, std::string> payload;
        // get current thread pid
        payload["pid"] = std::to_string(getpid());
        // request qos auth for current pid
        OHOS::ConcurrentTask::ConcurrentTaskClient::GetInstance().RequestAuth(payload);
    }
    return;
}

DataShareServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("data_share", []() {
        return std::make_shared<DataShareServiceImpl>();
    });
    staticActs_ = std::make_shared<DataShareStatic>();
    FeatureSystem::GetInstance().RegisterStaticActs("data_share", staticActs_);
}

DataShareServiceImpl::Factory::~Factory() {}

std::pair<int32_t, int32_t> DataShareServiceImpl::InsertEx(const std::string &uri, const std::string &extUri,
    const DataShareValuesBucket &valuesBucket)
{
    std::string func = __FUNCTION__;
    XCollie xcollie(func, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (GetSilentProxyStatus(uri, false) != E_OK) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return std::make_pair(ERROR, 0);
    }
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    auto callBack = [&uri, &valuesBucket, this, &callingTokenId, &func](ProviderInfo &providerInfo,
        DistributedData::StoreMetaData &metaData,
        std::shared_ptr<DBDelegate> dbDelegate) -> std::pair<int32_t, int32_t> {
        TimeoutReport timeoutReport({providerInfo.bundleName, providerInfo.moduleName, providerInfo.storeName, func,
            callingTokenId}, true);
        auto [errCode, ret] = dbDelegate->InsertEx(providerInfo.tableName, valuesBucket);
        if (errCode == E_OK && ret > 0) {
            NotifyChange(uri, providerInfo.visitedUserId);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.visitedUserId, metaData);
        }
        if (errCode != E_OK) {
            ReportExcuteFault(callingTokenId, providerInfo, errCode, func);
        }
        timeoutReport.Report();
        return std::make_pair(errCode, ret);
    };
    return ExecuteEx(uri, extUri, IPCSkeleton::GetCallingTokenID(), false, callBack);
}

bool DataShareServiceImpl::NotifyChange(const std::string &uri, int32_t userId)
{
    RadarReporter::RadarReport report(RadarReporter::NOTIFY_OBSERVER_DATA_CHANGE,
        RadarReporter::NOTIFY_DATA_CHANGE, __FUNCTION__);
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    if (obsMgrClient == nullptr) {
        ZLOGE("obsMgrClient is nullptr");
        report.SetError(RadarReporter::DATA_OBS_EMPTY_ERROR);
        return false;
    }
    ErrCode ret = obsMgrClient->NotifyChange(Uri(uri), userId);
    if (ret != ERR_OK) {
        ZLOGE("obsMgrClient->NotifyChange error return %{public}d", ret);
        report.SetError(RadarReporter::NOTIFY_ERROR);
        return false;
    }
    return true;
}

std::pair<int32_t, int32_t> DataShareServiceImpl::UpdateEx(const std::string &uri, const std::string &extUri,
    const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    std::string func = __FUNCTION__;
    XCollie xcollie(func, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (GetSilentProxyStatus(uri, false) != E_OK) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return std::make_pair(ERROR, 0);
    }
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    auto callBack = [&uri, &predicate, &valuesBucket, this, &callingTokenId, &func](ProviderInfo &providerInfo,
        DistributedData::StoreMetaData &metaData,
        std::shared_ptr<DBDelegate> dbDelegate) -> std::pair<int32_t, int32_t> {
        TimeoutReport timeoutReport({providerInfo.bundleName, providerInfo.moduleName, providerInfo.storeName, func,
            callingTokenId}, true);
        auto [errCode, ret] = dbDelegate->UpdateEx(providerInfo.tableName, predicate, valuesBucket);
        if (errCode == E_OK && ret > 0) {
            NotifyChange(uri, providerInfo.visitedUserId);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.visitedUserId, metaData);
        }
        if (errCode != E_OK) {
            ReportExcuteFault(callingTokenId, providerInfo, errCode, func);
        }
        timeoutReport.Report();
        return std::make_pair(errCode, ret);
    };
    return ExecuteEx(uri, extUri, callingTokenId, false, callBack);
}

std::pair<int32_t, int32_t> DataShareServiceImpl::DeleteEx(const std::string &uri, const std::string &extUri,
    const DataSharePredicates &predicate)
{
    std::string func = __FUNCTION__;
    XCollie xcollie(func, XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (GetSilentProxyStatus(uri, false) != E_OK) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return std::make_pair(ERROR, 0);
    }
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    auto callBack = [&uri, &predicate, this, &callingTokenId, &func](ProviderInfo &providerInfo,
        DistributedData::StoreMetaData &metaData,
        std::shared_ptr<DBDelegate> dbDelegate) -> std::pair<int32_t, int32_t> {
        TimeoutReport timeoutReport({providerInfo.bundleName, providerInfo.moduleName, providerInfo.storeName, func,
            callingTokenId}, true);
        auto [errCode, ret] = dbDelegate->DeleteEx(providerInfo.tableName, predicate);
        if (errCode == E_OK && ret > 0) {
            NotifyChange(uri, providerInfo.visitedUserId);
            RdbSubscriberManager::GetInstance().Emit(uri, providerInfo.visitedUserId, metaData);
        }
        if (errCode != E_OK) {
            ReportExcuteFault(callingTokenId, providerInfo, errCode, func);
        }
        timeoutReport.Report();
        return std::make_pair(errCode, ret);
    };
    return ExecuteEx(uri, extUri, callingTokenId, false, callBack);
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(const std::string &uri, const std::string &extUri,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns, int &errCode)
{
    std::string func = __FUNCTION__;
    XCollie xcollie(std::string(LOG_TAG) + "::" + func,
                    XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    if (GetSilentProxyStatus(uri, false) != E_OK) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return nullptr;
    }
    std::shared_ptr<DataShareResultSet> resultSet;
    auto callingPid = IPCSkeleton::GetCallingPid();
    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    auto callBack = [&uri, &predicates, &columns, &resultSet, &callingPid, &callingTokenId, &func, this]
        (ProviderInfo &providerInfo, DistributedData::StoreMetaData &,
        std::shared_ptr<DBDelegate> dbDelegate) -> std::pair<int32_t, int32_t> {
        TimeoutReport timeoutReport({providerInfo.bundleName, providerInfo.moduleName, providerInfo.storeName, func,
            callingTokenId}, true);
        auto [err, result] = dbDelegate->Query(providerInfo.tableName,
            predicates, columns, callingPid, callingTokenId);
        if (err != E_OK) {
            ReportExcuteFault(callingTokenId, providerInfo, err, func);
        }
        resultSet = std::move(result);
        timeoutReport.Report();
        return std::make_pair(err, E_OK);
    };
    auto [errVal, status] = ExecuteEx(uri, extUri, callingTokenId, true, callBack);
    errCode = errVal;
    return resultSet;
}

int32_t DataShareServiceImpl::AddTemplate(const std::string &uri, const int64_t subscriberId, const Template &tplt)
{
    auto context = std::make_shared<Context>(uri);
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    ZLOGI("Add template, uri %{private}s, subscriberId %{public}" PRIi64 ", bundleName %{public}s,"
          "predicates size %{public}zu.",
        uri.c_str(), subscriberId, tpltId.bundleName_.c_str(), tplt.predicates_.size());
    return templateStrategy_.Execute(context, [&uri, &tpltId, &tplt, &context]() -> int32_t {
        auto result = TemplateManager::GetInstance().Add(
            Key(uri, tpltId.subscriberId_, tpltId.bundleName_), context->visitedUserId, tplt);
        RdbSubscriberManager::GetInstance().Emit(context->uri, tpltId.subscriberId_, tpltId.bundleName_, context);
        return result;
    });
}

int32_t DataShareServiceImpl::DelTemplate(const std::string &uri, const int64_t subscriberId)
{
    auto context = std::make_shared<Context>(uri);
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    ZLOGI("Delete template, uri %{private}s, subscriberId %{public}" PRIi64 ", bundleName %{public}s.",
        URIUtils::Anonymous(uri).c_str(), subscriberId, tpltId.bundleName_.c_str());
    return templateStrategy_.Execute(context, [&uri, &tpltId, &context]() -> int32_t {
        return TemplateManager::GetInstance().Delete(
            Key(uri, tpltId.subscriberId_, tpltId.bundleName_), context->visitedUserId);
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

std::pair<bool, Security::AccessToken::ATokenTypeEnum> DataShareServiceImpl::GetCallerInfo(
    std::string &bundleName, int32_t &appIndex)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto type = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (type == Security::AccessToken::TOKEN_NATIVE) {
        return std::make_pair(true, type);
    }
    if (type != Security::AccessToken::TOKEN_HAP) {
        return std::make_pair(false, type);
    }
    Security::AccessToken::HapTokenInfo tokenInfo;
    auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result != Security::AccessToken::RET_SUCCESS) {
        ZLOGE("token:0x%{public}x, result:%{public}d", tokenId, result);
        return std::make_pair(false, type);
    }
    bundleName = tokenInfo.bundleName;
    appIndex = tokenInfo.instIndex;
    return std::make_pair(true, type);
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
            ZLOGE("publish error, key is %{public}s", URIUtils::Anonymous(item.key_).c_str());
            continue;
        }
        publishedData.emplace_back(context->uri, context->calledBundleName, item.subscriberId_);
        userId = context->visitedUserId;
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
                Key(context->uri, id.subscriberId_, id.bundleName_),
                observer, context, binderInfo_.executors);
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
                context->callerTokenId, context->visitedUserId);
        });
        results.emplace_back(uri, result);
        if (result == E_OK) {
            publishedKeys.emplace_back(context->uri, context->callerBundleName, subscriberId);
            if (binderInfo_.executors != nullptr) {
                binderInfo_.executors->Execute([context, subscriberId]() {
                    PublishedData::UpdateTimestamp(
                        context->uri, context->calledBundleName, subscriberId, context->visitedUserId);
                });
            }
            userId = context->visitedUserId;
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
                        context->uri, context->calledBundleName, subscriberId, context->visitedUserId);
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
                    context->uri, context->calledBundleName, subscriberId, context->visitedUserId);
            });
        }
        results.emplace_back(uri, result);
        if (result == E_OK) {
            PublishedDataKey pKey(context->uri, context->callerBundleName, subscriberId);
            if (PublishedDataSubscriberManager::GetInstance().IsNotifyOnEnabled(pKey, context->callerTokenId)) {
                publishedKeys.emplace_back(pKey);
            }
            userId = context->visitedUserId;
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
                        context->uri, context->calledBundleName, subscriberId, context->visitedUserId);
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
    const std::string accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    const auto userId = AccountDelegate::GetInstance()->GetUserByToken(binderInfo.selfTokenId);
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
    KvDBDelegate::GetInstance(saveMeta.dataDir, binderInfo.executors);
    SchedulerManager::GetInstance().SetExecutorPool(binderInfo.executors);
    NativeRdb::TaskExecutor::GetInstance().SetExecutor(binderInfo.executors);
    ExtensionAbilityManager::GetInstance().SetExecutorPool(binderInfo.executors);
    DBDelegate::SetExecutorPool(binderInfo.executors);
    HiViewAdapter::GetInstance().SetThreadPool(binderInfo.executors);
    SubscribeCommonEvent();
    SubscribeConcurrentTask();
    SubscribeTimeChanged();
    SubscribeChange();
    ZLOGI("end");
    return E_OK;
}

void DataShareServiceImpl::SubscribeCommonEvent()
{
    sptr<ISystemAbilityManager> systemManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemManager == nullptr) {
        ZLOGE("System mgr is nullptr");
        return;
    }
    sptr<SystemAbilityStatusChangeListener> callback(new (std::nothrow)SystemAbilityStatusChangeListener());
    if (callback == nullptr) {
        ZLOGE("new SystemAbilityStatusChangeListener failed! no memory");
        return;
    }
    systemManager->SubscribeSystemAbility(COMMON_EVENT_SERVICE_ID, callback);
}

void DataShareServiceImpl::SubscribeConcurrentTask()
{
    sptr<ISystemAbilityManager> systemManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemManager == nullptr) {
        ZLOGE("System mgr is nullptr");
        return;
    }
    sptr<SystemAbilityStatusChangeListener> callback(new (std::nothrow)SystemAbilityStatusChangeListener());
    if (callback == nullptr) {
        ZLOGE("new SystemAbilityStatusChangeListener failed! no memory");
        return;
    }
    systemManager->SubscribeSystemAbility(CONCURRENT_TASK_SERVICE_ID, callback);
}

void DataShareServiceImpl::SubscribeChange()
{
    EventCenter::GetInstance().Subscribe(RemoteChangeEvent::RDB_META_SAVE, [this](const Event &event) {
        auto &evt = static_cast<const RemoteChangeEvent &>(event);
        auto dataInfo = evt.GetDataInfo();
        SaveLaunchInfo(dataInfo.bundleName, dataInfo.userId, dataInfo.deviceId);
    });
    EventCenter::GetInstance().Subscribe(RemoteChangeEvent::DATA_CHANGE, [this](const Event &event) {
        AutoLaunch(event);
    });
}

void DataShareServiceImpl::SaveLaunchInfo(const std::string &bundleName, const std::string &userId,
    const std::string &deviceId)
{
    std::map<std::string, ProfileInfo> profileInfos;
    char *endptr = nullptr;
    errno = 0;
    long userIdLong = strtol(userId.c_str(), &endptr, DECIMAL_BASE);
    if (endptr == nullptr || endptr == userId.c_str() || *endptr != '\0') {
        ZLOGE("UserId:%{public}s is invalid", userId.c_str());
        return;
    }
    if (errno == ERANGE || userIdLong >= INT32_MAX || userIdLong <= INT32_MIN) {
        ZLOGE("UserId:%{public}s is out of range", userId.c_str());
        return;
    }
    int32_t currentUserId = static_cast<int32_t>(userIdLong);
    if (!DataShareProfileConfig::GetProfileInfo(bundleName, currentUserId, profileInfos)) {
        ZLOGE("Get profileInfo failed.");
        return;
    }
    if (profileInfos.empty()) {
        return;
    }
    for (auto &[uri, value] : profileInfos) {
        if (uri.find(EXT_URI_SCHEMA) == std::string::npos) {
            continue;
        }
        std::string extUri = uri;
        extUri.insert(strlen(EXT_URI_SCHEMA), "/");
        StoreMetaData meta = MakeMetaData(bundleName, userId, deviceId);
        if (value.launchInfos.empty()) {
            meta.storeId = "";
            AutoLaunchMetaData autoLaunchMetaData = {};
            std::vector<std::string> tempData = {};
            autoLaunchMetaData.datas.emplace(extUri, tempData);
            autoLaunchMetaData.launchForCleanData = value.launchForCleanData;
            MetaDataManager::GetInstance().SaveMeta(meta.GetAutoLaunchKey(), autoLaunchMetaData, true);
            ZLOGI("Without launchInfos, save meta end, bundleName = %{public}s.", bundleName.c_str());
            continue;
        }
        for (const auto &launchInfo : value.launchInfos) {
            AutoLaunchMetaData autoLaunchMetaData = {};
            autoLaunchMetaData.datas.emplace(extUri, launchInfo.tableNames);
            autoLaunchMetaData.launchForCleanData = value.launchForCleanData;
            meta.storeId = launchInfo.storeId;
            MetaDataManager::GetInstance().SaveMeta(meta.GetAutoLaunchKey(), autoLaunchMetaData, true);
        }
    }
}

bool DataShareServiceImpl::AllowCleanDataLaunchApp(const Event &event, bool launchForCleanData)
{
    auto &evt = static_cast<const RemoteChangeEvent &>(event);
    auto dataInfo = evt.GetDataInfo();
    // 1 means CLOUD_DATA_CLEAN
    if (dataInfo.changeType == 1) {
        return launchForCleanData; // Applications can be started by default
    }
    return true;
}

void DataShareServiceImpl::AutoLaunch(const Event &event)
{
    auto &evt = static_cast<const RemoteChangeEvent &>(event);
    auto dataInfo = evt.GetDataInfo();
    StoreMetaData meta = MakeMetaData(dataInfo.bundleName, dataInfo.userId, dataInfo.deviceId, dataInfo.storeId);
    AutoLaunchMetaData autoLaunchMetaData;
    if (!MetaDataManager::GetInstance().LoadMeta(std::move(meta.GetAutoLaunchKey()), autoLaunchMetaData, true)) {
        meta.storeId = "";
        if (!MetaDataManager::GetInstance().LoadMeta(std::move(meta.GetAutoLaunchKey()), autoLaunchMetaData, true)) {
            ZLOGE("No launch meta without storeId, bundleName = %{public}s.", dataInfo.bundleName.c_str());
            return;
        }
    }
    if (autoLaunchMetaData.datas.empty() || !AllowCleanDataLaunchApp(event, autoLaunchMetaData.launchForCleanData)) {
        return;
    }
    for (const auto &[uri, metaTables] : autoLaunchMetaData.datas) {
        if (dataInfo.tables.empty() && dataInfo.changeType == 1) {
            ZLOGI("Start to connect extension, bundlename = %{public}s.", dataInfo.bundleName.c_str());
            AAFwk::WantParams wantParams;
            ExtensionConnectAdaptor::TryAndWait(uri, dataInfo.bundleName, wantParams);
            return;
        }
        for (const auto &table : dataInfo.tables) {
            if (std::find(metaTables.begin(), metaTables.end(), table) != metaTables.end()) {
                ZLOGI("Find table, start to connect extension, bundlename = %{public}s.", dataInfo.bundleName.c_str());
                AAFwk::WantParams wantParams;
                ExtensionConnectAdaptor::TryAndWait(uri, dataInfo.bundleName, wantParams);
                break;
            }
        }
    }
}

StoreMetaData DataShareServiceImpl::MakeMetaData(const std::string &bundleName, const std::string &userId,
    const std::string &deviceId, const std::string storeId)
{
    StoreMetaData meta;
    meta.user = userId;
    meta.storeId = storeId;
    meta.deviceId = deviceId;
    meta.bundleName = bundleName;
    return meta;
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
    BundleMgrProxy::GetInstance()->Delete(bundleName, user, index);
    uint32_t tokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(user, bundleName, index);
    DBDelegate::EraseStoreCache(tokenId);
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    ZLOGI("AppExit uid=%{public}d, pid=%{public}d, tokenId=0x%{public}x, bundleName=%{public}s",
        uid, pid, tokenId, bundleName.c_str());
    RdbSubscriberManager::GetInstance().Delete(tokenId, pid);
    PublishedDataSubscriberManager::GetInstance().Delete(tokenId, pid);
    return E_OK;
}

int32_t DataShareServiceImpl::DataShareStatic::OnAppUpdate(const std::string &bundleName, int32_t user,
    int32_t index)
{
    ZLOGI("%{public}s updated", bundleName.c_str());
    BundleMgrProxy::GetInstance()->Delete(bundleName, user, index);
    std::string prefix = StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid, std::to_string(user), "default", bundleName });
    std::vector<StoreMetaData> storeMetaData;
    MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true);
    for (auto &meta : storeMetaData) {
        MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
    }
    SaveLaunchInfo(bundleName, std::to_string(user), DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid);
    return E_OK;
}

void DataShareServiceImpl::UpdateLaunchInfo()
{
    DataShareConfig *config = ConfigFactory::GetInstance().GetDataShareConfig();
    if (config == nullptr) {
        ZLOGE("DataShareConfig is nullptr");
        return;
    }

    ZLOGI("UpdateLaunchInfo begin.");
    for (auto &bundleName : config->updateLaunchNames) {
        std::string prefix = StoreMetaData::GetPrefix(
            { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid });
        std::vector<StoreMetaData> storeMetaData;
        MetaDataManager::GetInstance().LoadMeta(prefix, storeMetaData, true);
        std::vector<StoreMetaData> updateMetaData;
        for (auto &meta : storeMetaData) {
            if (meta.bundleName != bundleName) {
                continue;
            }
            updateMetaData.push_back(meta);
        }
        for (auto &meta : updateMetaData) {
            MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
        }
        for (auto &meta : updateMetaData) {
            BundleMgrProxy::GetInstance()->Delete(bundleName, atoi(meta.user.c_str()), 0);
            SaveLaunchInfo(meta.bundleName, meta.user, DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid);
        }
        ZLOGI("update bundleName %{public}s, size:%{public}zu.", bundleName.c_str(), storeMetaData.size());
    }
}

int32_t DataShareServiceImpl::DataShareStatic::OnClearAppStorage(const std::string &bundleName,
    int32_t user, int32_t index, int32_t tokenId)
{
    ZLOGI("ClearAppStorage user=%{public}d, index=%{public}d, token:0x%{public}x, bundleName=%{public}s",
        user, index, tokenId, bundleName.c_str());
    DBDelegate::EraseStoreCache(tokenId);
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("AppUninstall user=%{public}d, index=%{public}d, bundleName=%{public}s",
        user, index, bundleName.c_str());
    BundleMgrProxy::GetInstance()->Delete(bundleName, user, index);
    return E_OK;
}

int32_t DataShareServiceImpl::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    ZLOGI("AppUpdate user=%{public}d, index=%{public}d, bundleName=%{public}s",
        user, index, bundleName.c_str());
    BundleMgrProxy::GetInstance()->Delete(bundleName, user, index);
    return E_OK;
}

void DataShareServiceImpl::NotifyObserver(const std::string &uri)
{
    auto anonymous = URIUtils::Anonymous(uri);
    ZLOGD_MACRO("%{private}s try notified", anonymous.c_str());
    auto context = std::make_shared<Context>(uri);
    if (!GetCallerBundleName(context->callerBundleName)) {
        ZLOGE("get bundleName error, %{private}s", anonymous.c_str());
        return;
    }
    auto ret = rdbNotifyStrategy_.Execute(context);
    if (ret) {
        ZLOGI("%{private}s start notified", anonymous.c_str());
        RdbSubscriberManager::GetInstance().Emit(uri, context);
    }
}

bool DataShareServiceImpl::SubscribeTimeChanged()
{
    ZLOGD_MACRO("start");
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
    Handler handler = [] (int fd, std::map<std::string, std::vector<std::string>> &params) {
        DumpDataShareServiceInfo(fd, params);
    };
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
    SetServiceReady();
    ZLOGI("Init dataShare service end");
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
        ZLOGE("Enable silent proxy err, %{public}s", URIUtils::Anonymous(uri).c_str());
        return ERROR;
    }
    return E_OK;
}

int32_t DataShareServiceImpl::GetSilentProxyStatus(const std::string &uri, bool isCreateHelper)
{
    XCollie xcollie(std::string(LOG_TAG) + "::" + std::string(__FUNCTION__),
                    XCollie::XCOLLIE_LOG | XCollie::XCOLLIE_RECOVERY);
    uint32_t callerTokenId = IPCSkeleton::GetCallingTokenID();
    if (isCreateHelper) {
        auto errCode = GetBMSAndMetaDataStatus(uri, callerTokenId);
        if (errCode != E_OK) {
            ZLOGE("BMS or metaData not ready to complete, token:0x%{public}x, uri:%{public}s",
                callerTokenId, URIUtils::Anonymous(uri).c_str());
            return errCode;
        }
    }
    int32_t currentUserId = AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
    UriInfo uriInfo;
    // GetInfoFromUri will first perform a four-part URI check. Only if the URI contains more than four parts
    // is it necessary to continue to check the SilentProxyEnable status. The URI part length is used as an
    // indicator to determine whether the SilentProxyEnable status needs to be checked.
    if (!URIUtils::GetInfoFromURI(uri, uriInfo)) {
        return E_OK;
    }
    std::string calledBundleName = uriInfo.bundleName;
    int32_t appIndex = 0;
    if (!URIUtils::GetAppIndexFromProxyURI(uri, appIndex)) {
        return E_APPINDEX_INVALID;
    }
    uint32_t calledTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(
        currentUserId, calledBundleName, appIndex);
    if (calledTokenId == 0) {
        calledTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(0, calledBundleName, 0);
    }
    auto success = dataShareSilentConfig_.IsSilentProxyEnable(calledTokenId, currentUserId, calledBundleName, uri);
    if (!success) {
        ZLOGW("silent proxy disable, %{public}s", URIUtils::Anonymous(uri).c_str());
        return E_SILENT_PROXY_DISABLE;
    }
    return E_OK;
}

int32_t DataShareServiceImpl::RegisterObserver(const std::string &uri,
    const sptr<OHOS::IRemoteObject> &remoteObj)
{
    return ERROR;
}

int32_t DataShareServiceImpl::UnregisterObserver(const std::string &uri,
    const sptr<OHOS::IRemoteObject> &remoteObj)
{
    return ERROR;
}

bool DataShareServiceImpl::GetCallerBundleInfo(BundleInfo &callerBundleInfo)
{
    GetCallerInfo(callerBundleInfo.bundleName, callerBundleInfo.appIndex);
    callerBundleInfo.userId = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    callerBundleInfo.tokenId = IPCSkeleton::GetCallingTokenID();
    auto [ret, callerAppIdentifier] = BundleMgrProxy::GetInstance()->GetCallerAppIdentifier(
        callerBundleInfo.bundleName, callerBundleInfo.userId);
    if (ret != 0) {
        ZLOGE("Get caller appIdentifier failed, callerBundleName is %{public}s", callerBundleInfo.bundleName.c_str());
        return false;
    }
    callerBundleInfo.appIdentifier = callerAppIdentifier;
    return true;
}

std::vector<DataProxyResult> DataShareServiceImpl::PublishProxyData(const std::vector<DataShareProxyData> &proxyDatas,
    const DataProxyConfig &proxyConfig)
{
    std::vector<DataProxyResult> result;
    BundleInfo callerBundleInfo;
    if (!GetCallerBundleInfo(callerBundleInfo)) {
        ZLOGE("get callerBundleInfo failed");
        return result;
    }
    std::vector<ProxyDataKey> keys;
    std::map<DataShareObserver::ChangeType, std::vector<DataShareProxyData>> datas;
    for (const auto &data : proxyDatas) {
        DataShareObserver::ChangeType type;
        auto ret = PublishedProxyData::Upsert(data, callerBundleInfo, type);
        result.emplace_back(data.uri_, static_cast<DataProxyErrorCode>(ret));
        if (ret == SUCCESS &&
            (type == DataShareObserver::ChangeType::INSERT || type == DataShareObserver::ChangeType::UPDATE)) {
            keys.emplace_back(data.uri_, callerBundleInfo.bundleName);
            datas[type].emplace_back(data);
        }
    }
    ProxyDataSubscriberManager::GetInstance().Emit(keys, datas, callerBundleInfo.userId);
    return result;
}

std::vector<DataProxyResult> DataShareServiceImpl::DeleteProxyData(const std::vector<std::string> &uris,
    const DataProxyConfig &proxyConfig)
{
    std::vector<DataProxyResult> result;
    BundleInfo callerBundleInfo;
    if (!GetCallerBundleInfo(callerBundleInfo)) {
        ZLOGE("get callerBundleInfo failed");
        return result;
    }
    std::vector<ProxyDataKey> keys;
    std::map<DataShareObserver::ChangeType, std::vector<DataShareProxyData>> datas;
    for (const auto &uri : uris) {
        DataShareProxyData oldProxyData;
        DataShareObserver::ChangeType type;
        auto ret = PublishedProxyData::Delete(uri, callerBundleInfo, oldProxyData, type);
        result.emplace_back(uri, static_cast<DataProxyErrorCode>(ret));
        if (ret == SUCCESS && type == DataShareObserver::ChangeType::DELETE) {
            keys.emplace_back(uri, callerBundleInfo.bundleName);
            datas[type].emplace_back(oldProxyData);
        }
    }
    ProxyDataSubscriberManager::GetInstance().Emit(keys, datas, callerBundleInfo.userId);
    return result;
}

std::vector<DataProxyGetResult> DataShareServiceImpl::GetProxyData(const std::vector<std::string> &uris,
    const DataProxyConfig &proxyConfig)
{
    std::vector<DataProxyGetResult> result;
    BundleInfo callerBundleInfo;
    if (!GetCallerBundleInfo(callerBundleInfo)) {
        ZLOGE("get callerBundleInfo failed");
        return result;
    }
    for (const auto &uri : uris) {
        DataShareProxyData proxyData;
        auto ret = PublishedProxyData::Query(uri, callerBundleInfo, proxyData);
        result.emplace_back(uri, static_cast<DataProxyErrorCode>(ret), proxyData.value_, proxyData.allowList_);
    }
    return result;
}

std::vector<DataProxyResult> DataShareServiceImpl::SubscribeProxyData(const std::vector<std::string> &uris,
    const DataProxyConfig &proxyConfig, const sptr<IProxyDataObserver> observer)
{
    std::vector<DataProxyResult> results = {};
    BundleInfo callerBundleInfo;
    if (!GetCallerBundleInfo(callerBundleInfo)) {
        ZLOGE("get callerBundleInfo failed");
        return results;
    }
    for (const auto &uri : uris) {
        DataShareProxyData proxyData;
        DataProxyErrorCode ret =
            static_cast<DataProxyErrorCode>(PublishedProxyData::Query(uri, callerBundleInfo, proxyData));
        if (ret == SUCCESS) {
            ret = ProxyDataSubscriberManager::GetInstance().Add(
                ProxyDataKey(uri, callerBundleInfo.bundleName), observer, callerBundleInfo.bundleName,
                callerBundleInfo.appIdentifier, callerBundleInfo.userId);
        }
        results.emplace_back(uri, ret);
    }
    return results;
}

std::vector<DataProxyResult> DataShareServiceImpl::UnsubscribeProxyData(const std::vector<std::string> &uris,
    const DataProxyConfig &proxyConfig)
{
    std::string bundleName;
    GetCallerBundleName(bundleName);
    int32_t userId = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    std::vector<DataProxyResult> result = {};
    for (const auto &uri : uris) {
        auto ret = ProxyDataSubscriberManager::GetInstance().Delete(ProxyDataKey(uri, bundleName), userId);
        result.emplace_back(uri, ret);
    }
    return result;
}

bool DataShareServiceImpl::VerifyAcrossAccountsPermission(int32_t currentUserId, int32_t visitedUserId,
    const std::string &acrossAccountsPermission, uint32_t callerTokenId)
{
    // if it's SA, or visit itself, no need to verify permission
    if (currentUserId == 0 || currentUserId == visitedUserId) {
        return true;
    }
    return PermitDelegate::VerifyPermission(acrossAccountsPermission, callerTokenId);
}

std::pair<int32_t, int32_t> DataShareServiceImpl::ExecuteEx(const std::string &uri, const std::string &extUri,
    const int32_t tokenId, bool isRead, ExecuteCallbackEx callback)
{
    DataProviderConfig providerConfig(uri, tokenId);
    auto [errCode, providerInfo] = providerConfig.GetProviderInfo();
    if (errCode != E_OK) {
        ZLOGE("Provider failed! token:0x%{public}x,ret:%{public}d,uri:%{public}s,visitedUserId:%{public}d", tokenId,
            errCode, URIUtils::Anonymous(providerInfo.uri).c_str(), providerInfo.visitedUserId);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_GET_SUPPLIER,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::SUPPLIER_ERROR);
        return std::make_pair(errCode, 0);
    }
    // when HAP interacts across users, it needs to check across users permission
    if (!VerifyAcrossAccountsPermission(providerInfo.currentUserId, providerInfo.visitedUserId,
        providerInfo.acrossAccountsPermission, tokenId)) {
        ZLOGE("Across accounts permission denied! token:0x%{public}x, uri:%{public}s, current user:%{public}d,"
            "visited user:%{public}d", tokenId, URIUtils::Anonymous(providerInfo.uri).c_str(),
            providerInfo.currentUserId, providerInfo.visitedUserId);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_PERMISSION,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::PERMISSION_DENIED_ERROR);
        return std::make_pair(ERROR_PERMISSION_DENIED, 0);
    }
    if (!CheckAllowList(providerInfo.currentUserId, tokenId, providerInfo.allowLists)) {
        ZLOGE("CheckAllowList failed, permission denied! token:0x%{public}x, uri:%{public}s",
            tokenId, URIUtils::Anonymous(providerInfo.uri).c_str());
        return std::make_pair(ERROR_PERMISSION_DENIED, 0);
    }
    std::string permission = isRead ? providerInfo.readPermission : providerInfo.writePermission;
    if (!VerifyPermission(providerInfo.bundleName, permission, providerInfo.isFromExtension, tokenId)) {
        ZLOGE("Permission denied! token:0x%{public}x, permission:%{public}s,isFromExtension:%{public}d,uri:%{public}s",
            tokenId, permission.c_str(), providerInfo.isFromExtension, URIUtils::Anonymous(providerInfo.uri).c_str());
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_PERMISSION,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::PERMISSION_DENIED_ERROR);
        return std::make_pair(ERROR_PERMISSION_DENIED, 0);
    }
    DataShareDbConfig dbConfig;
    std::string extensionUri = extUri;
    if (extensionUri.empty()) {
        extensionUri = providerInfo.extensionUri;
    }
    DataShareDbConfig::DbConfig config {providerInfo.uri, extensionUri, providerInfo.bundleName,
        providerInfo.storeName, providerInfo.backup, providerInfo.singleton ? 0 : providerInfo.visitedUserId,
        providerInfo.appIndex, providerInfo.hasExtension};
    auto [code, metaData, dbDelegate] = dbConfig.GetDbConfig(config);
    if (code != E_OK) {
        ZLOGE("Get dbConfig fail,bundleName:%{public}s,tableName:%{public}s,tokenId:0x%{public}x, uri:%{public}s",
            providerInfo.bundleName.c_str(), StringUtils::GeneralAnonymous(providerInfo.tableName).c_str(), tokenId,
            URIUtils::Anonymous(providerInfo.uri).c_str());
        return std::make_pair(code, 0);
    }
    return callback(providerInfo, metaData, dbDelegate);
}

bool DataShareServiceImpl::CheckAllowList(const uint32_t &currentUserId, const uint32_t &callerTokenId,
    const std::vector<AllowList> &allowLists)
{
    if (allowLists.empty()) {
        return true;
    }
    std::string callerBundleName;
    int32_t callerAppIndex = 0;
    auto [success, type] = GetCallerInfo(callerBundleName, callerAppIndex);
    if (!success) {
        ZLOGE("Get caller info failed, token:0x%{public}x", callerTokenId);
        return false;
    }
    // SA calling
    if (type == Security::AccessToken::TOKEN_NATIVE) {
        return true;
    }

    auto [ret, callerAppIdentifier] = BundleMgrProxy::GetInstance()->GetCallerAppIdentifier(
        callerBundleName, currentUserId);
    if (ret != 0) {
        ZLOGE("Get caller appIdentifier failed, callerBundleName is %{public}s", callerBundleName.c_str());
        return false;
    }

    for (auto const& allowList : allowLists) {
        if (callerAppIdentifier == allowList.appIdentifier) {
            if (callerAppIndex != 0 && allowList.onlyMain) {
                ZLOGE("Provider only allow main application!");
                return false;
            }
            return true;
        }
    }
    return false;
}

int32_t DataShareServiceImpl::GetBMSAndMetaDataStatus(const std::string &uri, const int32_t tokenId)
{
    DataProviderConfig calledConfig(uri, tokenId);
    auto [errCode, calledInfo] = calledConfig.GetProviderInfo();
    if (errCode == E_URI_NOT_EXIST) {
        ZLOGE("Create helper invalid uri, token:0x%{public}x, uri:%{public}s", tokenId,
              URIUtils::Anonymous(calledInfo.uri).c_str());
        return E_OK;
    }
    if (errCode != E_OK) {
        ZLOGE("CalledInfo failed! token:0x%{public}x,ret:%{public}d,uri:%{public}s", tokenId,
            errCode, URIUtils::Anonymous(calledInfo.uri).c_str());
        return errCode;
    }
    DataShareDbConfig dbConfig;
    DataShareDbConfig::DbConfig dbArg;
    dbArg.uri = calledInfo.uri;
    dbArg.bundleName = calledInfo.bundleName;
    dbArg.storeName = calledInfo.storeName;
    dbArg.userId = calledInfo.singleton ? 0 : calledInfo.visitedUserId;
    dbArg.hasExtension = calledInfo.hasExtension;
    dbArg.appIndex = calledInfo.appIndex;
    auto [code, metaData] = dbConfig.GetMetaData(dbArg);
    if (code != E_OK) {
        ZLOGE("Get metaData fail,bundleName:%{public}s,tableName:%{public}s,tokenId:0x%{public}x, uri:%{public}s",
            calledInfo.bundleName.c_str(), StringUtils::GeneralAnonymous(calledInfo.tableName).c_str(), tokenId,
            URIUtils::Anonymous(calledInfo.uri).c_str());
        return E_METADATA_NOT_EXISTS;
    }
    return E_OK;
}

void DataShareServiceImpl::InitSubEvent()
{
    static std::atomic<bool> alreadySubscribe = false;
    bool except = false;
    if (!alreadySubscribe.compare_exchange_strong(except, true)) {
        return;
    }
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_CHANGED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    auto sysEventSubscriber = std::make_shared<SysEventSubscriber>(subscribeInfo, binderInfo_.executors);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(sysEventSubscriber)) {
        ZLOGE("Subscribe sys event failed.");
        alreadySubscribe = false;
    }
    if (BundleMgrProxy::GetInstance()->CheckBMS() != nullptr) {
        sysEventSubscriber->OnBMSReady();
    }
}

int32_t DataShareServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    ZLOGI("code:%{public}d, user:%{public}s, account:%{public}s", code, user.c_str(),
        Anonymous::Change(account).c_str());
    switch (code) {
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_DELETE):
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPED): {
            std::vector<int32_t> users;
            AccountDelegate::GetInstance()->QueryUsers(users);
            std::set<int32_t> userIds(users.begin(), users.end());
            userIds.insert(0);
            DBDelegate::Close([&userIds](const std::string &userId) {
                if (userIds.count(atoi(userId.c_str())) == 0) {
                    ZLOGW("Illegal use of database by user %{public}s", userId.c_str());
                    return true;
                }
                return false;
            });
            break;
        }
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPING):
            DBDelegate::Close([&user](const std::string &userId) {
                return user == userId;
            });
            break;
        default:
            break;
    }
    return E_OK;
}

void DataShareServiceImpl::ReportExcuteFault(uint32_t callingTokenId, DataProviderConfig::ProviderInfo &providerInfo,
    int32_t errCode, std::string &func)
{
    std::string appendix = "callingName:" + HiViewFaultAdapter::GetCallingName(callingTokenId).first;
    DataShareFaultInfo faultInfo = {HiViewFaultAdapter::curdFailed, providerInfo.bundleName, providerInfo.moduleName,
        providerInfo.storeName, func, errCode, appendix};
    HiViewFaultAdapter::ReportDataFault(faultInfo);
}

bool DataShareServiceImpl::VerifyPermission(const std::string &bundleName, const std::string &permission,
    bool isFromExtension, const int32_t tokenId)
{
    // Provider from extension, allows empty permissions, not configured to allow access.
    if (isFromExtension) {
        if (!permission.empty() && !PermitDelegate::VerifyPermission(permission, tokenId)) {
            return false;
        }
    } else {
        Security::AccessToken::HapTokenInfo tokenInfo;
        // Provider from ProxyData, which does not allow empty permissions and cannot be access without configured
        if (permission.empty()) {
            auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
            if (result == Security::AccessToken::RET_SUCCESS && tokenInfo.bundleName == bundleName) {
                return true;
            }
            ZLOGE("Permission denied! token:0x%{public}x, bundleName:%{public}s", tokenId, bundleName.c_str());
            return false;
        }
        // If the permission is NO_PERMISSION, access is also allowed
        if (permission != NO_PERMISSION && !PermitDelegate::VerifyPermission(permission, tokenId)) {
            return false;
        }
    }
    return true;
}
} // namespace OHOS::DataShare

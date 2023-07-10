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

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "app_connect_manager.h"
#include "dataobs_mgr_client.h"
#include "datashare_errno.h"
#include "datashare_template.h"
#include "directory/directory_manager.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "scheduler_manager.h"
#include "subscriber_managers/published_data_subscriber_manager.h"
#include "utils/anonymous.h"
#include "template_data.h"

namespace OHOS::DataShare {
using FeatureSystem = DistributedData::FeatureSystem;
__attribute__((used)) DataShareServiceImpl::Factory DataShareServiceImpl::factory_;
DataShareServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("data_share", []() {
        return std::make_shared<DataShareServiceImpl>();
    });
}

DataShareServiceImpl::Factory::~Factory() {}

int32_t DataShareServiceImpl::Insert(const std::string &uri, const DataShareValuesBucket &valuesBucket)
{
    ZLOGD("Insert enter.");
    auto context = std::make_shared<Context>(uri);
    auto ret = insertStrategy_.Execute(context, valuesBucket);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(
            uri, context->currentUserId, context->calledSourceDir, context->version);
    }
    return ret;
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
    auto context = std::make_shared<Context>(uri);
    auto ret = updateStrategy_.Execute(context, predicate, valuesBucket);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(
            uri, context->currentUserId, context->calledSourceDir, context->version);
    }
    return ret;
}

int32_t DataShareServiceImpl::Delete(const std::string &uri, const DataSharePredicates &predicate)
{
    ZLOGD("Delete enter.");
    auto context = std::make_shared<Context>(uri);
    auto ret = deleteStrategy_.Execute(context, predicate);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(
            uri, context->currentUserId, context->calledSourceDir, context->version);
    }
    return ret;
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(const std::string &uri,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns, int &errCode)
{
    ZLOGD("Query enter.");
    auto context = std::make_shared<Context>(uri);
    return queryStrategy_.Execute(context, predicates, columns, errCode);
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
    return templateStrategy_.Execute(context, [&uri, &tpltId, &tplt, &context]() -> int32_t {
        auto result = TemplateManager::GetInstance().Add(
            Key(uri, tpltId.subscriberId_, tpltId.bundleName_), context->currentUserId, tplt);
        RdbSubscriberManager::GetInstance().Emit(context->uri, context);
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
        if (result != EOK) {
            ZLOGE("publish error, key is %{public}s", DistributedData::Anonymous::Change(item.key_).c_str());
            continue;
        }
        publishedData.emplace_back(context->uri, context->calledBundleName, item.subscriberId_);
        userId = context->currentUserId;
    }
    if (!publishedData.empty()) {
        PublishedDataSubscriberManager::GetInstance().Emit(publishedData, userId, callerBundleName);
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
            publishedKeys.emplace_back(context->uri, context->callerBundleName, subscriberId);
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
    saveMeta.area = 1;
    saveMeta.uid = IPCSkeleton::GetCallingUid();
    saveMeta.storeType = DATA_SHARE_SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    KvDBDelegate::GetInstance(false, saveMeta.dataDir, binderInfo.executors);
    SchedulerManager::GetInstance().SetExecutorPool(binderInfo.executors);
    return EOK;
}

int32_t DataShareServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    RdbSubscriberManager::GetInstance().Clear();
    PublishedDataSubscriberManager::GetInstance().Clear();
    SchedulerManager::GetInstance().ClearTimer();
    return EOK;
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

int32_t DataShareServiceImpl::OnAppUninstall(
    const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    ZLOGI("%{public}s uninstalled", bundleName.c_str());
    PublishedData::Delete(bundleName, user);
    PublishedData::ClearAging();
    TemplateData::Delete(bundleName, user);
    RdbHelper::ClearCache();
    return EOK;
}

int32_t DataShareServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName)
{
    ZLOGI("AppExit uid=%{public}d, pid=%{public}d, tokenId=0x%{public}x, bundleName=%{public}s",
        uid, pid, tokenId, bundleName.c_str());
    RdbSubscriberManager::GetInstance().Delete(tokenId);
    PublishedDataSubscriberManager::GetInstance().Delete(tokenId);
    return EOK;
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
} // namespace OHOS::DataShare

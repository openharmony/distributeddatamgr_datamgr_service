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
#include "delete_strategy.h"
#include "directory/directory_manager.h"
#include "get_data_strategy.h"
#include "hap_token_info.h"
#include "insert_strategy.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "query_strategy.h"
#include "scheduler_manager.h"
#include "subscribe_strategy.h"
#include "template_manager.h"
#include "update_strategy.h"
#include "utils/anonymous.h"

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
    auto ret = InsertStrategy::Execute(context, valuesBucket);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(uri, context->calledSourceDir, context->version);
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
    auto ret = UpdateStrategy::Execute(context, predicate, valuesBucket);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(uri, context->calledSourceDir, context->version);
    }
    return ret;
}

int32_t DataShareServiceImpl::Delete(const std::string &uri, const DataSharePredicates &predicate)
{
    ZLOGD("Delete enter.");
    auto context = std::make_shared<Context>(uri);
    auto ret = DeleteStrategy::Execute(context, predicate);
    if (ret) {
        NotifyChange(uri);
        RdbSubscriberManager::GetInstance().Emit(uri, context);
        SchedulerManager::GetInstance().Execute(uri, context->calledSourceDir, context->version);
    }
    return ret;
}

std::shared_ptr<DataShareResultSet> DataShareServiceImpl::Query(const std::string &uri,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns, int &errCode)
{
    ZLOGD("Query enter.");
    auto context = std::make_shared<Context>(uri);
    return QueryStrategy::Execute(context, predicates, columns, errCode);
}

int32_t DataShareServiceImpl::AddTemplate(const std::string &uri, const int64_t subscriberId, const Template &tplt)
{
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    return TemplateManager::GetInstance().AddTemplate(uri, tpltId, tplt);
}

int32_t DataShareServiceImpl::DelTemplate(const std::string &uri, const int64_t subscriberId)
{
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    return TemplateManager::GetInstance().DelTemplate(uri, tpltId);
}

bool DataShareServiceImpl::GetCallerBundleName(std::string &bundleName)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId) != Security::AccessToken::TOKEN_HAP) {
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
    GetCallerBundleName(callerBundleName);
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
        publishedData.emplace_back(context->uri, callerBundleName, item.subscriberId_);
    }
    PublishedDataSubscriberManager::GetInstance().Emit(publishedData, callerBundleName);
    return results;
}

Data DataShareServiceImpl::GetData(const std::string &bundleNameOfProvider)
{
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    auto context = std::make_shared<Context>();
    context->callerBundleName = callerBundleName;
    context->calledBundleName = bundleNameOfProvider;
    return getDataStrategy_.Execute(context);
}

std::vector<OperationResult> DataShareServiceImpl::SubscribeRdbData(
    const std::vector<std::string> &uris, const TemplateId &id, const sptr<IDataProxyRdbObserver> observer)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(
            uri, SubscribeStrategy::Execute(context, [&id, &observer, &context, this]() -> bool {
                return RdbSubscriberManager::GetInstance().AddRdbSubscriber(
                    context->uri, id, observer, context, binderInfo_.executors);
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
        results.emplace_back(uri, SubscribeStrategy::Execute(context, [&id, &context]() -> bool {
            return RdbSubscriberManager::GetInstance().DelRdbSubscriber(context->uri, id, context->callerTokenId);
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
        results.emplace_back(uri, SubscribeStrategy::Execute(context, [&id, &context]() -> bool {
            return RdbSubscriberManager::GetInstance().EnableRdbSubscriber(context->uri, id, context);
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
        results.emplace_back(uri, SubscribeStrategy::Execute(context, [&id, &context]() -> bool {
            return RdbSubscriberManager::GetInstance().DisableRdbSubscriber(context->uri, id, context->callerTokenId);
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::SubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    std::vector<PublishedDataKey> publishedKeys;
    int32_t result;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        result = SubscribeStrategy::Execute(
            context, [&subscriberId, &observer, &callerBundleName, &context]() -> bool {
                return PublishedDataSubscriberManager::GetInstance().AddSubscriber(
                    context->uri, callerBundleName, subscriberId, observer, context->callerTokenId);
            });
        results.emplace_back(uri, result);
        if (result == E_OK) {
            publishedKeys.emplace_back(key);
        }
    }
    PublishedDataSubscriberManager::GetInstance().Emit(publishedKeys, callerBundleName, observer);
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::UnsubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        results.emplace_back(
            uri, SubscribeStrategy::Execute(context, [&subscriberId, &callerBundleName, &context]() -> bool {
                return PublishedDataSubscriberManager::GetInstance().DelSubscriber(
                    context->uri, callerBundleName, subscriberId, context->callerTokenId);
            }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::EnablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    std::vector<PublishedDataKey> publishedKeys;
    int32_t result;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        result = SubscribeStrategy::Execute(context, [&subscriberId, &callerBundleName, &context]() -> bool {
            return PublishedDataSubscriberManager::GetInstance().EnableSubscriber(
                context->uri, callerBundleName, subscriberId, context->callerTokenId);
        });
        results.emplace_back(uri, result);
        if (result == E_OK) {
            publishedKeys.emplace_back(key);
        }
    }
    PublishedDataSubscriberManager::GetInstance().Emit(publishedKeys, callerBundleName);
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::DisablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    std::vector<OperationResult> results;
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        PublishedDataKey key(uri, callerBundleName, subscriberId);
        context->callerBundleName = callerBundleName;
        context->calledBundleName = key.bundleName;
        results.emplace_back(
            uri, SubscribeStrategy::Execute(context, [&subscriberId, &callerBundleName, &context]() -> bool {
                return PublishedDataSubscriberManager::GetInstance().DisableSubscriber(
                    context->uri, callerBundleName, subscriberId, context->callerTokenId);
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
    return EOK;
}

int32_t DataShareServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    const std::string accountId = DistributedKv::AccountDelegate::GetInstance()->GetCurrentAccountId();
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "default";
    saveMeta.storeId = "data_share_data_";
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName =  binderInfo_.selfName;
    saveMeta.appId = binderInfo_.selfName;
    saveMeta.user = user;
    saveMeta.account = account;
    saveMeta.tokenId = binderInfo_.selfTokenId;
    saveMeta.securityLevel = DistributedKv::SecurityLevel::S1;
    saveMeta.area = 1;
    saveMeta.uid = IPCSkeleton::GetCallingUid();
    saveMeta.storeType = DATA_SHARE_SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    KvDBDelegate::GetInstance(true, saveMeta.dataDir, binderInfo_.executors);
    return EOK;
}

void DataShareServiceImpl::OnConnectDone()
{
    std::string callerBundleName;
    GetCallerBundleName(callerBundleName);
    AppConnectManager::Notify(callerBundleName);
}
} // namespace OHOS::DataShare

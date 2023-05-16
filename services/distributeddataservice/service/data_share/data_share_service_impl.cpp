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
#include "bootstrap.h"
#include "dataobs_mgr_client.h"
#include "datashare_errno.h"
#include "datashare_template.h"
#include "delete_strategy.h"
#include "directory_manager.h"
#include "get_data_strategy.h"
#include "hap_token_info.h"
#include "insert_strategy.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "publish_strategy.h"
#include "query_strategy.h"
#include "subscribe_strategy.h"
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
        return ret;
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
        return ret;
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
        return ret;
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
    return ERROR;
}

int32_t DataShareServiceImpl::DelTemplate(const std::string &uri, const int64_t subscriberId)
{
    TemplateId tpltId;
    tpltId.subscriberId_ = subscriberId;
    if (!GetCallerBundleName(tpltId.bundleName_)) {
        ZLOGE("get bundleName error, %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return ERROR;
    }
    return ERROR;
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
    return std::vector<OperationResult>();
}

Data DataShareServiceImpl::GetData(const std::string &bundleNameOfProvider)
{
    return Data();
}

std::vector<OperationResult> DataShareServiceImpl::SubscribeRdbData(
    const std::vector<std::string> &uris, const TemplateId &id, const sptr<IDataProxyRdbObserver> observer)
{
    return std::vector<OperationResult>();
}

std::vector<OperationResult> DataShareServiceImpl::UnsubscribeRdbData(
    const std::vector<std::string> &uris, const TemplateId &id)
{
    std::vector<OperationResult> results;
    for (const auto &uri : uris) {
        auto context = std::make_shared<Context>(uri);
        results.emplace_back(uri, SubscribeStrategy::Execute(context, [&id, &context]() -> bool {
            return true;
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
            return true;
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
            return true;
        }));
    }
    return results;
}

std::vector<OperationResult> DataShareServiceImpl::SubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId, const sptr<IDataProxyPublishedDataObserver> observer)
{
    return std::vector<OperationResult>();
}

std::vector<OperationResult> DataShareServiceImpl::UnsubscribePublishedData(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    return std::vector<OperationResult>();
}

std::vector<OperationResult> DataShareServiceImpl::EnablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    return std::vector<OperationResult>();
}

std::vector<OperationResult> DataShareServiceImpl::DisablePubSubs(const std::vector<std::string> &uris,
    const int64_t subscriberId)
{
    return std::vector<OperationResult>();
}

enum DataShareKvStoreType : int32_t {
    DATA_SHARE_SINGLE_VERSION = 0,
    DISTRIBUTED_TYPE_BUTT
};

int32_t DataShareServiceImpl::OnInitialize()
{
    auto token = IPCSkeleton::GetCallingTokenID();
    const std::string accountId = DistributedKv::AccountDelegate::GetInstance()->GetCurrentAccountId();
    const auto userId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(token);
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "default";
    saveMeta.storeId = "data_share_data_";
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.appId =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.user = std::to_string(userId);
    saveMeta.account = accountId;
    saveMeta.tokenId = token;
    saveMeta.securityLevel = DistributedKv::SecurityLevel::S1;
    saveMeta.area = 1;
    saveMeta.uid = IPCSkeleton::GetCallingUid();
    saveMeta.storeType = DATA_SHARE_SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    KvDBDelegate::GetInstance(false, saveMeta.dataDir);
    return EOK;
}

int32_t DataShareServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    auto token = IPCSkeleton::GetCallingTokenID();
    const std::string accountId = DistributedKv::AccountDelegate::GetInstance()->GetCurrentAccountId();
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "default";
    saveMeta.storeId = "data_share_data_";
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.appId =  DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.user = user;
    saveMeta.account = account;
    saveMeta.tokenId = token;
    saveMeta.securityLevel = DistributedKv::SecurityLevel::S1;
    saveMeta.area = 1;
    saveMeta.uid = IPCSkeleton::GetCallingUid();
    saveMeta.storeType = DATA_SHARE_SINGLE_VERSION;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);
    KvDBDelegate::GetInstance(true, saveMeta.dataDir);
    return EOK;
}
} // namespace OHOS::DataShare

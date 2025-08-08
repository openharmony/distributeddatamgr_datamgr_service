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

#define LOG_TAG "ProxyDataManager"

#include "datashare_observer.h"
#include "data_share_profile_config.h"
#include "dataproxy_handle_common.h"
#include "log_print.h"
#include "proxy_data_manager.h"
#include "utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {

ProxyDataManager &ProxyDataManager::GetInstance()
{
    static ProxyDataManager instance;
    return instance;
}

ProxyDataList::ProxyDataList(const ProxyDataListNode &node)
    : KvData(Id(std::to_string(node.tokenId), node.userId)), value(node)
{
}

bool ProxyDataList::HasVersion() const
{
    return true;
}

int ProxyDataList::GetVersion() const
{
    return value.GetVersion();
}

std::string ProxyDataList::GetValue() const
{
    return DistributedData::Serializable::Marshall(value);
}

PublishedProxyData::PublishedProxyData(const ProxyDataNode &node)
    : KvData(Id(node.proxyData.uri, node.userId)), value(node)
{
}

bool PublishedProxyData::HasVersion() const
{
    return true;
}

int PublishedProxyData::GetVersion() const
{
    return value.GetVersion();
}

std::string PublishedProxyData::GetValue() const
{
    return DistributedData::Serializable::Marshall(value);
}

ProxyDataListNode::ProxyDataListNode() : VersionData(-1) {}

ProxyDataNode::ProxyDataNode() : VersionData(-1) {}

bool ProxyDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(proxyData)], proxyData);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    ret = ret && SetValue(node[GET_NAME(tokenId)], tokenId);
    return ret && VersionData::Marshal(node);
}

bool ProxyDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(proxyData), proxyData);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    return ret && GetValue(node, GET_NAME(tokenId), tokenId);
}

bool ProxyDataListNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(uris)], uris);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    ret = ret && SetValue(node[GET_NAME(tokenId)], tokenId);
    return ret && VersionData::Marshal(node);
}

bool ProxyDataListNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(uris), uris);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    ret = ret && GetValue(node, GET_NAME(tokenId), tokenId);
    return ret;
}

bool PublishedProxyData::VerifyPermission(const BundleInfo &callerBundleInfo, const ProxyDataNode &data)
{
    if (callerBundleInfo.tokenId == data.tokenId) {
        return true;
    }

    for (const auto &item : data.proxyData.allowList) {
        if (callerBundleInfo.appIdentifier == item) {
            return true;
        }
    }
    return false;
}

bool PublishedProxyData::CheckAndCorrectProxyData(DataShareProxyData &proxyData)
{
    // the upper limit of value is 4096 bytes, only string type is possible to exceed the limit
    if (proxyData.value_.index() == DataProxyValueType::VALUE_STRING) {
        std::string valueStr = std::get<std::string>(proxyData.value_);
        if (valueStr.size() > VALUE_MAX_SIZE) {
            ZLOGE("value of proxyData %{public}s is over limit, size %{public}zu",
                URIUtils::Anonymous(proxyData.uri_).c_str(), valueStr.size());
            return false;
        }
    }
    if (proxyData.uri_.size() > URI_MAX_SIZE) {
        ZLOGE("uri of proxyData %{public}s is over limit, size %{public}zu",
            URIUtils::Anonymous(proxyData.uri_).c_str(), proxyData.uri_.size());
        return false;
    }
    int32_t allowListCount = 1;
    auto it = proxyData.allowList_.begin();
    while (it != proxyData.allowList_.end()) {
        if (it->size() > APPIDENTIFIER_MAX_SIZE) {
            ZLOGW("appidentifier of proxyData %{public}s is over limit, size %{public}zu",
                URIUtils::Anonymous(proxyData.uri_).c_str(), it->size());
            it = proxyData.allowList_.erase(it);
        } else {
            if (allowListCount++ > ALLOW_LIST_MAX_COUNT) {
                break;
            }
            it++;
        }
    }
    if (proxyData.allowList_.size() > ALLOW_LIST_MAX_COUNT) {
        ZLOGW("allowList of proxyData %{public}s is over limit, size %{public}zu",
            URIUtils::Anonymous(proxyData.uri_).c_str(), proxyData.allowList_.size());
        proxyData.allowList_.resize(ALLOW_LIST_MAX_COUNT);
    }
    return true;
}

int32_t PublishedProxyData::PutIntoTable(std::shared_ptr<KvDBDelegate> kvDelegate, int32_t user,
    uint32_t tokenId, const std::vector<std::string> &proxyDataList, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }

    SerialDataShareProxyData serialProxyData(proxyData.uri_, proxyData.value_, proxyData.allowList_);
    auto ret = kvDelegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, PublishedProxyData(ProxyDataNode(
        serialProxyData, user, tokenId)));
    if (ret.first != E_OK) {
        ZLOGE("db Upsert failed, %{public}s %{public}d",
            URIUtils::Anonymous(proxyData.uri_).c_str(), ret.first);
        return INNER_ERROR;
    }

    if (!proxyDataList.empty()) {
        auto value = ProxyDataList(ProxyDataListNode(
            proxyDataList, user, tokenId));
        ret = kvDelegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, value);
        if (ret.first != E_OK) {
            ZLOGE("db Upsert failed, %{public}x %{public}d", tokenId, user);
            return INNER_ERROR;
        }
    }
    return SUCCESS;
}

int32_t PublishedProxyData::InsertProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
    const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }

    ProxyDataListNode proxyDataList;
    std::string listFilter = Id(std::to_string(tokenId), user);
    std::string listQueryResult;
    kvDelegate->Get(KvDBDelegate::PROXYDATA_TABLE, listFilter, "{}", listQueryResult);
    if (!listQueryResult.empty()) {
        if (!ProxyDataListNode::Unmarshall(listQueryResult, proxyDataList)) {
            ZLOGE("ProxyDataListNode unmarshall failed, %{public}s",
                StringUtils::GeneralAnonymous(listQueryResult).c_str());
            return INNER_ERROR;
        }
    }
    if (proxyDataList.uris.size() >= PROXY_DATA_MAX_COUNT) {
        return OVER_LIMIT;
    }

    std::string modifyBundle;
    URIUtils::GetBundleNameFromProxyURI(proxyData.uri_, modifyBundle);
    if (modifyBundle != bundleName) {
        ZLOGE("only allowed to publish the proxyData of self bundle %{public}s, dest bundle %{public}s",
            bundleName.c_str(), modifyBundle.c_str());
        return NO_PERMISSION;
    }

    auto it = std::find(proxyDataList.uris.begin(), proxyDataList.uris.end(), proxyData.uri_);
    if (it == proxyDataList.uris.end()) {
        proxyDataList.uris.emplace_back(proxyData.uri_);
    }
    return PutIntoTable(kvDelegate, user, tokenId, proxyDataList.uris, proxyData);
}

int32_t PublishedProxyData::UpdateProxyData(std::shared_ptr<KvDBDelegate> kvDelegate, const std::string &bundleName,
    const int32_t &user, const uint32_t &tokenId, const DataShareProxyData &proxyData)
{
    if (kvDelegate == nullptr) {
        ZLOGE("kv delegate is null!");
        return INNER_ERROR;
    }
    std::vector<std::string> proxyDataList;
    return PutIntoTable(kvDelegate, user, tokenId, proxyDataList, proxyData);
}

int32_t PublishedProxyData::Query(const std::string &uri, const BundleInfo &callerBundleInfo,
    DataShareProxyData &proxyData)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }
    std::string filter = Id(uri, callerBundleInfo.userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return URI_NOT_EXIST;
    }
    ProxyDataNode data;
    if (!ProxyDataNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return INNER_ERROR;
    }
    DataShareProxyData tempProxyData(data.proxyData.uri, data.proxyData.value, data.proxyData.allowList);
    if (callerBundleInfo.tokenId != data.tokenId) {
        tempProxyData.allowList_ = {};
    }
    if (!VerifyPermission(callerBundleInfo, data)) {
        return NO_PERMISSION;
    }

    proxyData = tempProxyData;
    return SUCCESS;
}

int32_t PublishedProxyData::Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
    DataShareObserver::ChangeType &type)
{
    type = DataShareObserver::ChangeType::INVAILD;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }

    auto data = proxyData;
    if (!CheckAndCorrectProxyData(data)) {
        return OVER_LIMIT;
    }
    std::string filter = Id(data.uri_, callerBundleInfo.userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        type = DataShareObserver::ChangeType::INSERT;
        return InsertProxyData(delegate, callerBundleInfo.bundleName,
            callerBundleInfo.userId, callerBundleInfo.tokenId, data);
    } else {
        ProxyDataNode oldData;
        if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
            ZLOGE("ProxyDataNode unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
            return INNER_ERROR;
        }
        if (callerBundleInfo.tokenId != oldData.tokenId) {
            ZLOGE("only allow to modify the proxyData of self bundle %{public}d, dest bundle %{public}d",
                callerBundleInfo.tokenId, oldData.tokenId);
            return NO_PERMISSION;
        }
        if (data.isValueUndefined) {
            data.value_ = oldData.proxyData.value;
        }
        if (data.isAllowListUndefined) {
            data.allowList_ = oldData.proxyData.allowList;
        }
        // only when value changed is need notify
        if (oldData.proxyData.value != data.value_) {
            type = DataShareObserver::ChangeType::UPDATE;
        }
        return UpdateProxyData(delegate, callerBundleInfo.bundleName,
            callerBundleInfo.userId, callerBundleInfo.tokenId, data);
    }
}

int32_t PublishedProxyData::Delete(const std::string &uri, const BundleInfo &callerBundleInfo,
    DataShareProxyData &oldProxyData, DataShareObserver::ChangeType &type)
{
    type = DataShareObserver::ChangeType::INVAILD;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return INNER_ERROR;
    }

    std::string bundleName;
    URIUtils::GetBundleNameFromProxyURI(uri, bundleName);
    if (callerBundleInfo.bundleName != bundleName) {
        return NO_PERMISSION;
    }
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, Id(uri, callerBundleInfo.userId), "{}", queryResult);
    if (queryResult.empty()) {
        return URI_NOT_EXIST;
    }

    ProxyDataNode oldData;
    if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return INNER_ERROR;
    }
    if (callerBundleInfo.tokenId != oldData.tokenId) {
        return NO_PERMISSION;
    }
    auto ret = delegate->Delete(KvDBDelegate::PROXYDATA_TABLE, Id(uri, callerBundleInfo.userId));
    if (ret.first != E_OK) {
        ZLOGE("db Delete failed, %{public}s %{public}d", URIUtils::Anonymous(uri).c_str(), ret.first);
        return INNER_ERROR;
    }
    std::vector<std::string> uris;
    auto count = ProxyDataList::Query(callerBundleInfo.tokenId, callerBundleInfo.userId, uris);
    if (count <= 0) {
        ZLOGI("get bundle %{public}s's proxyData failed", callerBundleInfo.bundleName.c_str());
        return INNER_ERROR;
    }
    auto it = std::find(uris.begin(), uris.end(), uri);
    if (it != uris.end()) {
        uris.erase(it);
    }
    ret = delegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, ProxyDataList(ProxyDataListNode(
        uris, callerBundleInfo.userId, callerBundleInfo.tokenId)));
    if (ret.first != E_OK) {
        ZLOGE("db Upsert failed, %{public}x %{public}d", callerBundleInfo.tokenId, ret.first);
        return INNER_ERROR;
    }
    oldProxyData = DataShareProxyData(oldData.proxyData.uri, oldData.proxyData.value, oldData.proxyData.allowList);
    type = DataShareObserver::ChangeType::DELETE;
    return SUCCESS;
}

int32_t ProxyDataList::Query(uint32_t tokenId, int32_t userId, std::vector<std::string> &proxyDataList)
{
    int32_t invalidNum = -1;
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        return invalidNum;
    }
    std::string filter = Id(std::to_string(tokenId), userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return invalidNum;
    }
    ProxyDataListNode data;
    if (!ProxyDataListNode::Unmarshall(queryResult, data)) {
        ZLOGE("Unmarshall failed, %{public}s", StringUtils::GeneralAnonymous(queryResult).c_str());
        return invalidNum;
    }

    proxyDataList = std::move(data.uris);
    return proxyDataList.size();
}

bool ProxyDataManager::GetCrossAppSharedConfig(const std::string &bundleName, int32_t user,
    int32_t index, std::vector<DataShareProxyData> &proxyDatas)
{
    BundleConfig bundleConfig;
    auto ret = BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(bundleName, user, bundleConfig, index);
    if (ret != E_OK) {
        ZLOGE("Get bundleInfo falied, ret : %{public}d", ret);
        return false;
    }
    std::vector<std::string> resourcePath;
    std::vector<SerialDataShareProxyData> datas;
    std::for_each(bundleConfig.hapModuleInfos.begin(),
        bundleConfig.hapModuleInfos.end(), [&datas](const HapModuleInfo hapModuleInfo) {
        if (!hapModuleInfo.crossAppSharedConfig.resourcePath.empty()) {
            auto [ret, data] = DataShareProfileConfig::GetCrossAppSharedConfig(
                hapModuleInfo.crossAppSharedConfig.resourcePath, hapModuleInfo.resourcePath, hapModuleInfo.hapPath);
            if (ret == SUCCESS) {
                datas.insert(datas.end(), data.begin(), data.end());
            } else {
                ZLOGE("get shareConfig failed, err: %{public}d", ret);
            }
        }
    });
    std::for_each(datas.begin(), datas.end(), [&proxyDatas](auto &data) {
        proxyDatas.emplace_back(std::move(data.uri), std::move(data.value), std::move(data.allowList));
    });
    return true;
}

void ProxyDataManager::OnAppInstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    std::vector<DataShareProxyData> proxyDatas;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    if (!GetCrossAppSharedConfig(bundleName, user, index, proxyDatas)) {
        ZLOGE("GetCrossAppSharedConfig after install failed");
        return;
    }
    DataShareObserver::ChangeType type;
    std::for_each(proxyDatas.begin(), proxyDatas.end(),
        [user, bundleName, callerBundleInfo, &type](const DataShareProxyData proxyData) {
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type);
    });
}

void ProxyDataManager::OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    std::vector<DataShareProxyData> proxyDatas;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    DataShareObserver::ChangeType type;
    if (!GetCrossAppSharedConfig(bundleName, user, index, proxyDatas)) {
        ZLOGE("GetCrossAppSharedConfig after install failed");
        return;
    }
    std::vector<std::string> uris;
    auto count = ProxyDataList::Query(tokenId, user, uris);
    if (count <= 0) {
        ZLOGI("bundle %{public}s has no proxyData", bundleName.c_str());
    }
    DataShareProxyData oldProxyData;
    for (const auto &uri : uris) {
        PublishedProxyData::Delete(uri, callerBundleInfo, oldProxyData, type);
    }
    std::for_each(proxyDatas.begin(), proxyDatas.end(),
        [user, bundleName, callerBundleInfo, &type](const DataShareProxyData proxyData) {
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type);
    });
}

void ProxyDataManager::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    DataShareProxyData proxyData;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    DataShareObserver::ChangeType type;
    std::vector<std::string> uris;
    auto count = ProxyDataList::Query(tokenId, user, uris);
    if (count <= 0) {
        ZLOGI("bundle %{public}s has no proxyData", bundleName.c_str());
    }
    for (const auto &uri : uris) {
        PublishedProxyData::Delete(uri, callerBundleInfo, proxyData, type);
    }
}
} // namespace OHOS::DataShare
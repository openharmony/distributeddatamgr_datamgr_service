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
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {

ProxyDataManager &ProxyDataManager::GetInstance()
{
    static ProxyDataManager instance;
    return instance;
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

ProxyDataNode::ProxyDataNode() : VersionData(-1) {}

bool ProxyDataNode::Marshal(DistributedData::Serializable::json &node) const
{
    bool ret = SetValue(node[GET_NAME(proxyData)], proxyData);
    ret = ret && SetValue(node[GET_NAME(bundleName)], bundleName);
    ret = ret && SetValue(node[GET_NAME(userId)], userId);
    ret = ret && SetValue(node[GET_NAME(appIndex)], appIndex);
    ret = ret && SetValue(node[GET_NAME(tokenId)], tokenId);
    return ret && VersionData::Marshal(node);
}

bool ProxyDataNode::Unmarshal(const DistributedData::Serializable::json &node)
{
    bool ret = GetValue(node, GET_NAME(proxyData), proxyData);
    ret = ret && GetValue(node, GET_NAME(bundleName), bundleName);
    ret = ret && GetValue(node, GET_NAME(userId), userId);
    ret = ret && GetValue(node, GET_NAME(appIndex), appIndex);
    return ret && GetValue(node, GET_NAME(tokenId), tokenId);
}

bool PublishedProxyData::VerifySelfAccess(const BundleInfo &callerBundleInfo,
    const ProxyDataNode &data, bool isInstallEvent)
{
    std::string bundleName;
    URIUtils::GetBundleNameFromProxyURI(data.proxyData.uri, bundleName);
    if (callerBundleInfo.bundleName == bundleName && callerBundleInfo.appIndex == data.appIndex &&
        callerBundleInfo.userId == data.userId) {
        return true;
    }
    ZLOGE("only allow to modify the proxyData of self bundle");
    return false;
}

bool PublishedProxyData::VerifyPermission(const BundleInfo &callerBundleInfo, const ProxyDataNode &data)
{
    if (VerifySelfAccess(callerBundleInfo, data)) {
        return true;
    }

    for (const auto &item : data.proxyData.allowList) {
        if (callerBundleInfo.appIdentifier == item || item == ALLOW_ALL) {
            return true;
        }
    }
    return false;
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
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return INNER_ERROR;
    }
    DataShareProxyData tempProxyData(data.proxyData.uri, data.proxyData.value, data.proxyData.allowList);
    if (!VerifySelfAccess(callerBundleInfo, data)) {
        tempProxyData.allowList_ = {};
    }
    if (!VerifyPermission(callerBundleInfo, data)) {
        return NO_PERMISSION;
    }

    proxyData = tempProxyData;
    return SUCCESS;
}

int32_t PublishedProxyData::Upsert(const DataShareProxyData &proxyData, const BundleInfo &callerBundleInfo,
    DataShareObserver::ChangeType &type, bool isInstallEvent)
{
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        type = DataShareObserver::ChangeType::INVAILD;
        return INNER_ERROR;
    }

    std::string filter = Id(proxyData.uri_, callerBundleInfo.userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);

    if (queryResult.empty()) {
        type = DataShareObserver::ChangeType::INSERT;
        std::string modifyBundle;
        URIUtils::GetBundleNameFromProxyURI(proxyData.uri_, modifyBundle);
        if (modifyBundle != callerBundleInfo.bundleName) {
            ZLOGE("only allowed to modify the proxyData of self bundle");
            return NO_PERMISSION;
        }
    } else {
        ProxyDataNode oldData;
        if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
            ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
            return INNER_ERROR;
        }
        if (!VerifySelfAccess(callerBundleInfo, oldData, isInstallEvent)) {
            return NO_PERMISSION;
        }
    }
    SerialDataShareProxyData serialProxyData(proxyData.uri_, proxyData.value_, proxyData.allowList_);
    auto [status, count] = delegate->Upsert(KvDBDelegate::PROXYDATA_TABLE, PublishedProxyData(ProxyDataNode(
        serialProxyData, callerBundleInfo.bundleName, callerBundleInfo.userId,
        callerBundleInfo.appIndex, callerBundleInfo.tokenId)));
    if (status != E_OK) {
        ZLOGE("db Upsert failed, %{public}s %{public}d",
            URIUtils::Anonymous(proxyData.uri_).c_str(), status);
        type = DataShareObserver::ChangeType::INVAILD;
        return INNER_ERROR;
    }
    type = DataShareObserver::ChangeType::UPDATE;
    return SUCCESS;
}

int32_t PublishedProxyData::Delete(const std::string &uri, const BundleInfo &callerBundleInfo,
    DataShareProxyData &oldProxyData, DataShareObserver::ChangeType &type, bool isInstallEvent)
{
    std::string bundleName;
    URIUtils::GetBundleNameFromProxyURI(uri, bundleName);
    if (callerBundleInfo.bundleName != bundleName) {
        return NO_PERMISSION;
    }
    auto delegate = KvDBDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("db open failed");
        type = DataShareObserver::ChangeType::INVAILD;
        return INNER_ERROR;
    }

    std::string filter = Id(uri, callerBundleInfo.userId);
    std::string queryResult;
    delegate->Get(KvDBDelegate::PROXYDATA_TABLE, filter, "{}", queryResult);
    if (queryResult.empty()) {
        return URI_NOT_EXIST;
    }

    ProxyDataNode oldData;
    if (!ProxyDataNode::Unmarshall(queryResult, oldData)) {
        ZLOGE("Unmarshall failed, %{private}s", queryResult.c_str());
        return INNER_ERROR;
    }
    if (!VerifySelfAccess(callerBundleInfo, oldData, isInstallEvent)) {
        return NO_PERMISSION;
    }
    oldProxyData.uri_ = oldData.proxyData.uri;
    oldProxyData.value_ = oldData.proxyData.value;
    oldProxyData.allowList_ = oldData.proxyData.allowList;
    auto [status, count] = delegate->Delete(KvDBDelegate::PROXYDATA_TABLE, filter);
    if (status != E_OK) {
        ZLOGE("db Delete failed, %{public}s %{public}d", filter.c_str(), status);
        type = DataShareObserver::ChangeType::INVAILD;
        return INNER_ERROR;
    }
    type = DataShareObserver::ChangeType::DELETE;
    return SUCCESS;
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
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type, true);
    });
}

void ProxyDataManager::OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    std::vector<DataShareProxyData> proxyDatas;
    BundleInfo callerBundleInfo;
    callerBundleInfo.bundleName = bundleName;
    callerBundleInfo.userId = user;
    callerBundleInfo.appIndex = index;
    callerBundleInfo.tokenId = tokenId;
    DataShareObserver::ChangeType type;
    std::for_each(proxyDatas.begin(), proxyDatas.end(),
        [user, bundleName, callerBundleInfo, &type](const DataShareProxyData proxyData) {
        PublishedProxyData::Upsert(proxyData, callerBundleInfo, type, true);
    });
}
} // namespace OHOS::DataShare
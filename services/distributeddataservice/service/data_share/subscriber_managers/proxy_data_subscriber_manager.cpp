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

#define LOG_TAG "ProxyDataSubscriberManager"

#include "proxy_data_subscriber_manager.h"

#include "dataproxy_handle_common.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "proxy_data_manager.h"
#include "utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
ProxyDataSubscriberManager &ProxyDataSubscriberManager::GetInstance()
{
    static ProxyDataSubscriberManager manager;
    return manager;
}

DataProxyErrorCode ProxyDataSubscriberManager::Add(const ProxyDataKey &key, const sptr<IProxyDataObserver> &observer,
    const std::string &bundleName, const std::string &callerAppIdentifier, const int32_t &userId)
{
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    ProxyDataCache_.Compute(
        key, [&observer, callerTokenId, bundleName, callerAppIdentifier, userId](const ProxyDataKey &key,
        std::vector<ObserverNode> &value) {
            ZLOGI("add proxy data subscriber, uri %{public}s",
                URIUtils::Anonymous(key.uri).c_str());
            value.emplace_back(observer, callerTokenId, bundleName, callerAppIdentifier, userId);
            return true;
        });
    return SUCCESS;
}

DataProxyErrorCode ProxyDataSubscriberManager::Delete(const ProxyDataKey &key, const int32_t &userId)
{
    auto callerTokenId = IPCSkeleton::GetCallingTokenID();
    auto result =
        ProxyDataCache_.ComputeIfPresent(key, [&userId, callerTokenId](const auto &key,
            std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end();) {
                if (it->callerTokenId == callerTokenId && it->userId == userId) {
                    ZLOGI("delete proxy data subscriber, uri %{public}s",
                        URIUtils::Anonymous(key.uri).c_str());
                    it = value.erase(it);
                } else {
                    it++;
                }
            }
            return !value.empty();
        });
    return result ? SUCCESS : URI_NOT_EXIST;
}

bool ProxyDataSubscriberManager::CheckAllowList(const std::vector<std::string> &allowList,
    const std::string &callerAppIdentifier)
{
    for (const auto &item : allowList) {
        if (callerAppIdentifier == item || item == ALLOW_ALL) {
            return true;
        }
    }
    return false;
}

// if arg observer is not null, notify that observer only; otherwise notify all observers
void ProxyDataSubscriberManager::Emit(const std::vector<ProxyDataKey> &keys,
    const std::map<DataShareObserver::ChangeType, std::vector<DataShareProxyData>> &datas, const int32_t &userId)
{
    std::map<sptr<IProxyDataObserver>, std::vector<DataProxyChangeInfo>> callbacks;
    for (auto typeIt = datas.begin(); typeIt != datas.end(); ++typeIt) {
        auto type = typeIt->first;
        for (const auto &data : typeIt->second) {
            std::string bundleName;
            URIUtils::GetBundleNameFromProxyURI(data.uri_, bundleName);
            auto it = ProxyDataCache_.Find(ProxyDataKey(data.uri_, bundleName));
            if (!it.first) {
                continue;
            }
            auto &observers = it.second;
            std::for_each(observers.begin(), observers.end(),
                [&callbacks, data, userId, bundleName, &type, this](const auto &obs) {
                if ((CheckAllowList(data.allowList_, obs.callerAppIdentifier) ||
                    bundleName == obs.bundleName) && obs.userId == userId) {
                    callbacks[obs.observer].emplace_back(type, data.uri_, data.value_);
                } else {
                    ZLOGE("no permission to receive notification");
                }
            });
        }
    }
    for (auto &[callback, changeInfo] : callbacks) {
        if (callback != nullptr) {
            callback->OnChangeFromProxyData(changeInfo);
        }
    }
}

void ProxyDataSubscriberManager::Clear()
{
    ProxyDataCache_.Clear();
}

ProxyDataKey::ProxyDataKey(const std::string &uri, const std::string &bundle)
    : uri(uri), bundleName(bundle)
{
    /* private published data can use key as simple uri */
    /* etc: datashareproxy://{bundleName}/meeting can use meeting replaced */
    /* if key is normal uri, bundleName is from uri */
    if (URIUtils::IsDataProxyURI(uri)) {
        URIUtils::GetBundleNameFromProxyURI(uri, bundleName);
    }
}

bool ProxyDataKey::operator<(const ProxyDataKey &rhs) const
{
    return uri < rhs.uri;
}

bool ProxyDataKey::operator>(const ProxyDataKey &rhs) const
{
    return rhs < *this;
}

bool ProxyDataKey::operator<=(const ProxyDataKey &rhs) const
{
    return !(rhs < *this);
}

bool ProxyDataKey::operator>=(const ProxyDataKey &rhs) const
{
    return !(*this < rhs);
}

bool ProxyDataKey::operator==(const ProxyDataKey &rhs) const
{
    return uri == rhs.uri && bundleName == rhs.bundleName;
}

bool ProxyDataKey::operator!=(const ProxyDataKey &rhs) const
{
    return !(rhs == *this);
}

ProxyDataSubscriberManager::ObserverNode::ObserverNode(const sptr<IProxyDataObserver> &observer,
    const uint32_t &callerTokenId, const std::string &bundleName,
    const std::string &callerAppIdentifier, const int32_t &userId) : observer(observer),
    callerTokenId(callerTokenId), bundleName(bundleName), callerAppIdentifier(callerAppIdentifier), userId(userId)
{
}
} // namespace OHOS::DataShare

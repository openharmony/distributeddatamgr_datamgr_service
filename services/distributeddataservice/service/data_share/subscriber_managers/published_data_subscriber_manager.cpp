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
#define LOG_TAG "PublishedDataSubscriberManager"

#include "published_data_subscriber_manager.h"

#include <cinttypes>

#include "ipc_skeleton.h"
#include "general/load_config_data_info_strategy.h"
#include "log_print.h"
#include "published_data.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
PublishedDataSubscriberManager &PublishedDataSubscriberManager::GetInstance()
{
    static PublishedDataSubscriberManager manager;
    return manager;
}

int PublishedDataSubscriberManager::Add(
    const PublishedDataKey &key, const sptr<IDataProxyPublishedDataObserver> observer, uint32_t firstCallerTokenId)
{
    publishedDataCache_.Compute(
        key, [&observer, &firstCallerTokenId, this](const PublishedDataKey &key, std::vector<ObserverNode> &value) {
            ZLOGI("add publish subscriber, uri %{public}s tokenId 0x%{public}x",
                DistributedData::Anonymous::Change(key.key).c_str(), firstCallerTokenId);
            value.emplace_back(observer, firstCallerTokenId, IPCSkeleton::GetCallingTokenID());
            return true;
        });
    return E_OK;
}

int PublishedDataSubscriberManager::Delete(const PublishedDataKey &key, uint32_t firstCallerTokenId)
{
    auto result =
        publishedDataCache_.ComputeIfPresent(key, [&firstCallerTokenId](const auto &key,
            std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end();) {
                if (it->firstCallerTokenId == firstCallerTokenId) {
                    ZLOGI("delete publish subscriber, uri %{public}s tokenId 0x%{public}x",
                        DistributedData::Anonymous::Change(key.key).c_str(), firstCallerTokenId);
                    it = value.erase(it);
                } else {
                    it++;
                }
            }
            return !value.empty();
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void PublishedDataSubscriberManager::Delete(uint32_t callerTokenId)
{
    publishedDataCache_.EraseIf([&callerTokenId](const auto &key, std::vector<ObserverNode> &value) {
        for (auto it = value.begin(); it != value.end();) {
            if (it->callerTokenId == callerTokenId) {
                ZLOGI("erase start, uri is %{public}s, tokenId is 0x%{public}x",
                    DistributedData::Anonymous::Change(key.key).c_str(), callerTokenId);
                it = value.erase(it);
            } else {
                it++;
            }
        }
        return value.empty();
    });
}

int PublishedDataSubscriberManager::Disable(const PublishedDataKey &key, uint32_t firstCallerTokenId)
{
    auto result =
        publishedDataCache_.ComputeIfPresent(key, [&firstCallerTokenId](const auto &key,
            std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->firstCallerTokenId == firstCallerTokenId) {
                    it->enabled = false;
                    it->isNotifyOnEnabled = false;
                }
            }
            return true;
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

int PublishedDataSubscriberManager::Enable(const PublishedDataKey &key, uint32_t firstCallerTokenId)
{
    auto result =
        publishedDataCache_.ComputeIfPresent(key, [&firstCallerTokenId](const auto &key,
            std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (it->firstCallerTokenId == firstCallerTokenId) {
                    it->enabled = true;
                }
            }
            return true;
        });
    return result ? E_OK : E_SUBSCRIBER_NOT_EXIST;
}

void PublishedDataSubscriberManager::Emit(const std::vector<PublishedDataKey> &keys, int32_t userId,
    const std::string &ownerBundleName, const sptr<IDataProxyPublishedDataObserver> observer)
{
    int32_t status;
    // key is bundleName, value is change node
    std::map<PublishedDataKey, PublishedDataNode::Data> publishedResult;
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> callbacks;
    publishedDataCache_.ForEach([&keys, &status, &observer, &publishedResult, &callbacks, &userId, this](
        const PublishedDataKey &key, std::vector<ObserverNode> &val) {
        for (auto &data : keys) {
            if (key != data || publishedResult.count(key) != 0) {
                continue;
            }
            status = PublishedData::Query(
                Id(PublishedData::GenId(key.key, key.bundleName, key.subscriberId), userId), publishedResult[key]);
            if (status != E_OK) {
                ZLOGE("query fail %{public}s %{public}s %{public}" PRId64, data.bundleName.c_str(), data.key.c_str(),
                    data.subscriberId);
                publishedResult.erase(key);
                continue;
            }
            PutInto(callbacks, val, key, observer);
            break;
        }
        return false;
    });
    PublishedDataChangeNode result;
    for (auto &[callback, keys] : callbacks) {
        result.datas_.clear();
        for (auto &key : keys) {
            if (publishedResult.count(key) != 0) {
                result.datas_.emplace_back(key.key, key.subscriberId, PublishedDataNode::MoveTo(publishedResult[key]));
            }
        }
        if (result.datas_.empty()) {
            continue;
        }
        result.ownerBundleName_ = ownerBundleName;
        callback->OnChangeFromPublishedData(result);
    }
}

void PublishedDataSubscriberManager::PutInto(
    std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> &callbacks,
    const std::vector<ObserverNode> &val, const PublishedDataKey &key,
    const sptr<IDataProxyPublishedDataObserver> observer)
{
    for (auto const &callback : val) {
        if (callback.enabled && callback.observer != nullptr) {
            // callback the observer, others do not call
            if (observer != nullptr && callback.observer != observer) {
                continue;
            }
            callbacks[callback.observer].emplace_back(key);
        }
    }
}

void PublishedDataSubscriberManager::Clear()
{
    publishedDataCache_.Clear();
}

int PublishedDataSubscriberManager::GetCount(const PublishedDataKey &key)
{
    int count = 0;
    publishedDataCache_.ComputeIfPresent(key, [&count](const auto &key, std::vector<ObserverNode> &value) {
        count = static_cast<int>(value.size());
        return true;
    });
    return count;
}

bool PublishedDataSubscriberManager::IsNotifyOnEnabled(const PublishedDataKey &key, uint32_t callerTokenId)
{
    auto pair = publishedDataCache_.Find(key);
    if (!pair.first) {
        return false;
    }
    for (const auto &value : pair.second) {
        if (value.firstCallerTokenId == callerTokenId && value.isNotifyOnEnabled) {
            return true;
        }
    }
    return false;
}

void PublishedDataSubscriberManager::SetObserversNotifiedOnEnabled(const std::vector<PublishedDataKey> &keys)
{
    for (const auto &pkey : keys) {
        publishedDataCache_.ComputeIfPresent(pkey, [](const auto &key, std::vector<ObserverNode> &value) {
            for (auto it = value.begin(); it != value.end(); it++) {
                if (!it->enabled) {
                    it->isNotifyOnEnabled = true;
                }
            }
            return true;
        });
    }
}

PublishedDataKey::PublishedDataKey(const std::string &key, const std::string &bundle, const int64_t subscriberId)
    : key(key), bundleName(bundle), subscriberId(subscriberId)
{
    /* private published data can use key as simple uri */
    /* etc: datashareproxy://{bundleName}/meeting can use meeting replaced */
    /* if key is normal uri, bundleName is from uri */
    if (URIUtils::IsDataProxyURI(key)) {
        URIUtils::GetBundleNameFromProxyURI(key, bundleName);
    }
}

bool PublishedDataKey::operator<(const PublishedDataKey &rhs) const
{
    if (key < rhs.key) {
        return true;
    }
    if (rhs.key < key) {
        return false;
    }
    if (bundleName < rhs.bundleName) {
        return true;
    }
    if (rhs.bundleName < bundleName) {
        return false;
    }
    return subscriberId < rhs.subscriberId;
}

bool PublishedDataKey::operator>(const PublishedDataKey &rhs) const
{
    return rhs < *this;
}

bool PublishedDataKey::operator<=(const PublishedDataKey &rhs) const
{
    return !(rhs < *this);
}

bool PublishedDataKey::operator>=(const PublishedDataKey &rhs) const
{
    return !(*this < rhs);
}

bool PublishedDataKey::operator==(const PublishedDataKey &rhs) const
{
    return key == rhs.key && bundleName == rhs.bundleName && subscriberId == rhs.subscriberId;
}

bool PublishedDataKey::operator!=(const PublishedDataKey &rhs) const
{
    return !(rhs == *this);
}

PublishedDataSubscriberManager::ObserverNode::ObserverNode(const sptr<IDataProxyPublishedDataObserver> &observer,
    uint32_t firstCallerTokenId, uint32_t callerTokenId)
    : observer(observer), firstCallerTokenId(firstCallerTokenId), callerTokenId(callerTokenId)
{
}
} // namespace OHOS::DataShare

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

#ifndef DATASHARESERVICE_PUBLISHED_DATA_SUBSCRIBER_MANAGER_H
#define DATASHARESERVICE_PUBLISHED_DATA_SUBSCRIBER_MANAGER_H

#include <list>
#include <string>

#include "concurrent_map.h"
#include "context.h"
#include "data_proxy_observer.h"
#include "datashare_template.h"
#include "executor_pool.h"
namespace OHOS::DataShare {
struct PublishedDataKey {
    PublishedDataKey(const std::string &key, const std::string &bundleName, const int64_t subscriberId);
    bool operator<(const PublishedDataKey &rhs) const;
    bool operator>(const PublishedDataKey &rhs) const;
    bool operator<=(const PublishedDataKey &rhs) const;
    bool operator>=(const PublishedDataKey &rhs) const;
    bool operator==(const PublishedDataKey &rhs) const;
    bool operator!=(const PublishedDataKey &rhs) const;
    std::string key;
    std::string bundleName;
    int64_t subscriberId;
};

class PublishedDataSubscriberManager {
public:
    static PublishedDataSubscriberManager &GetInstance();
    int Add(const PublishedDataKey &key, const sptr<IDataProxyPublishedDataObserver> observer,
        const uint32_t callerTokenId);
    int Delete(const PublishedDataKey &key, const uint32_t callerTokenId);
    int Disable(const PublishedDataKey &key, const uint32_t callerTokenId);
    int Enable(const PublishedDataKey &key, const uint32_t callerTokenId);
    void Emit(const std::vector<PublishedDataKey> &keys, const int32_t userId, const std::string &ownerBundleName,
        const sptr<IDataProxyPublishedDataObserver> observer = nullptr);
    void Clear();

private:
    struct ObserverNode {
        ObserverNode(const sptr<IDataProxyPublishedDataObserver> &observer, uint32_t callerTokenId);
        sptr<IDataProxyPublishedDataObserver> observer;
        uint32_t callerTokenId;
        bool enabled = true;
    };
    class ObserverNodeRecipient : public IRemoteObject::DeathRecipient {
    public:
        ObserverNodeRecipient(PublishedDataSubscriberManager *owner, const PublishedDataKey &key,
            sptr<IDataProxyPublishedDataObserver> observer)
            : owner_(owner), key_(key), observer_(observer){};

        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            if (owner_ != nullptr) {
                owner_->OnRemoteDied(key_, observer_);
            }
        }

    private:
        PublishedDataSubscriberManager *owner_;
        PublishedDataKey key_;
        sptr<IDataProxyPublishedDataObserver> observer_;
    };

    void LinkToDeath(const PublishedDataKey &key, sptr<IDataProxyPublishedDataObserver> observer);
    void OnRemoteDied(const PublishedDataKey &key, sptr<IDataProxyPublishedDataObserver> observer);

    PublishedDataSubscriberManager() = default;
    void PutInto(std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> &,
        const std::vector<ObserverNode> &, const PublishedDataKey &, const sptr<IDataProxyPublishedDataObserver>);
    ConcurrentMap<PublishedDataKey, std::vector<ObserverNode>> publishedDataCache;
};
} // namespace OHOS::DataShare
#endif

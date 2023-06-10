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

#ifndef DATASHARESERVICE_TEMPLATE_MANAGER_H
#define DATASHARESERVICE_TEMPLATE_MANAGER_H

#include <list>
#include <string>

#include "concurrent_map.h"
#include "context.h"
#include "data_proxy_observer.h"
#include "datashare_template.h"
#include "executor_pool.h"
namespace OHOS::DataShare {
struct Key {
    Key(const std::string &uri, const int64_t subscriberId, const std::string &bundleName);
    bool operator==(const Key &rhs) const;
    bool operator!=(const Key &rhs) const;
    bool operator<(const Key &rhs) const;
    bool operator>(const Key &rhs) const;
    bool operator<=(const Key &rhs) const;
    bool operator>=(const Key &rhs) const;
    const std::string uri;
    const int64_t subscriberId;
    const std::string bundleName;
};
class TemplateManager {
public:
    static TemplateManager &GetInstance();
    int32_t  Add(const Key &key, const int32_t userId, const Template &tpl);
    int32_t  Delete(const Key &key, const int32_t userId);
    bool Get(const Key &key, const int32_t userId, Template &tpl);

private:
    TemplateManager();
    friend class RdbSubscriberManager;
};

class RdbSubscriberManager {
public:
    static RdbSubscriberManager &GetInstance();
    int Add(const Key &key, const sptr<IDataProxyRdbObserver> observer, std::shared_ptr<Context> context,
        std::shared_ptr<ExecutorPool> executorPool);
    int Delete(const Key &key, const uint32_t callerTokenId);
    int Disable(const Key &key, const uint32_t callerTokenId);
    int Enable(const Key &key, std::shared_ptr<Context> context);
    void Emit(const std::string &uri, std::shared_ptr<Context> context);
    void EmitByKey(const Key &key, const int32_t userId, const std::string &rdbPath, int version);
    int GetCount(const Key &key);
    std::vector<Key> GetKeysByUri(const std::string &uri);
    void Clear();
private:
    struct ObserverNode {
        ObserverNode(const sptr<IDataProxyRdbObserver> &observer, uint32_t callerTokenId);
        sptr<IDataProxyRdbObserver> observer;
        uint32_t callerTokenId;
        bool enabled = true;
    };

    class ObserverNodeRecipient : public IRemoteObject::DeathRecipient {
    public:
        ObserverNodeRecipient(RdbSubscriberManager *owner, const Key &key,
            sptr<IDataProxyRdbObserver> observer) : owner_(owner), key_(key), observer_(observer) {};

        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            if (owner_ != nullptr) {
                owner_->OnRemoteDied(key_, observer_);
            }
        }

    private:
        RdbSubscriberManager *owner_;
        Key key_;
        sptr<IDataProxyRdbObserver> observer_;
    };

    void LinkToDeath(const Key &key, sptr<IDataProxyRdbObserver> observer);
    void OnRemoteDied(const Key &key, sptr<IDataProxyRdbObserver> observer);
    RdbSubscriberManager() = default;
    ConcurrentMap<Key, std::vector<ObserverNode>> rdbCache_;
    int Notify(const Key &key, const int32_t userId, const std::vector<ObserverNode> &val, const std::string &rdbDir,
        int rdbVersion);
    int GetEnableObserverCount(const Key &key);
};

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
            sptr<IDataProxyPublishedDataObserver> observer) : owner_(owner), key_(key), observer_(observer) {};

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

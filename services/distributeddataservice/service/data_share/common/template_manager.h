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
#include "datashare_template.h"
#include "data_proxy_observer.h"
#include "context.h"
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
    bool AddTemplate(const std::string &uri, const TemplateId &tplId, const Template &tpl);
    bool DelTemplate(const std::string &uri, const TemplateId &tplId);
    bool GetTemplate(const std::string &uri, int64_t subscriberId, const std::string &bundleName, Template &tpl);

private:
    TemplateManager();
    friend class RdbSubscriberManager;
};

class RdbSubscriberManager {
public:
    static RdbSubscriberManager &GetInstance();
    int AddRdbSubscriber(const std::string &uri, const TemplateId &tplId, const sptr<IDataProxyRdbObserver> observer,
        std::shared_ptr<Context> context);
    int DelRdbSubscriber(const std::string &uri, const TemplateId &tplId, const uint32_t callerTokenId);
    int DisableRdbSubscriber(
        const std::string &uri, const TemplateId &tplId, const uint32_t callerTokenId);
    int EnableRdbSubscriber(const std::string &uri, const TemplateId &tplId, std::shared_ptr<Context> context);
    void Emit(const std::string &uri, std::shared_ptr<Context> context);
    void EmitByKey(const Key &key, const std::string &rdbPath, int version);
    int GetObserverCount(const Key &key);
    std::vector<Key> GetKeysByUri(const std::string &uri);

private:
    struct ObserverNode {
        ObserverNode(const sptr<IDataProxyRdbObserver> &observer, uint32_t callerTokenId);
        sptr<IDataProxyRdbObserver> observer;
        uint32_t callerTokenId;
        bool enabled = true;
    };

    RdbSubscriberManager() = default;
    ConcurrentMap<Key, std::vector<ObserverNode>> rdbCache_;
    int Notify(const Key &key, std::vector<ObserverNode> &val, const std::string &rdbDir, int rdbVersion);
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
    int AddSubscriber(const std::string &key, const std::string &callerBundleName, const int64_t subscriberId,
        const sptr<IDataProxyPublishedDataObserver> observer, const uint32_t callerTokenId);
    int DelSubscriber(const std::string &uri, const std::string &callerBundleName, const int64_t subscriberId,
        const uint32_t callerTokenId);
    int DisableSubscriber(const std::string &uri, const std::string &callerBundleName, const int64_t subscriberId,
        const uint32_t callerTokenId);
    int EnableSubscriber(const std::string &uri, const std::string &callerBundleName, const int64_t subscriberId,
        const uint32_t callerTokenId);
    void Emit(const std::vector<PublishedDataKey> &keys, const std::string &ownerBundleName,
        const sptr<IDataProxyPublishedDataObserver> observer = nullptr);
private:
    struct ObserverNode {
        ObserverNode(const sptr<IDataProxyPublishedDataObserver> &observer, uint32_t callerTokenId);
        sptr<IDataProxyPublishedDataObserver> observer;
        uint32_t callerTokenId;
        bool enabled = true;
    };
    PublishedDataSubscriberManager() = default;
    void PutInto(std::map<sptr<IDataProxyPublishedDataObserver>, std::vector<PublishedDataKey>> &,
        const std::vector<ObserverNode> &, const PublishedDataKey &, const sptr<IDataProxyPublishedDataObserver>);
    ConcurrentMap<PublishedDataKey, std::vector<ObserverNode>> publishedDataCache;
};
} // namespace OHOS::DataShare
#endif

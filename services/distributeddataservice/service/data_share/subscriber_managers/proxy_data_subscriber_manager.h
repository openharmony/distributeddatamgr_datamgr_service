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

#ifndef DATASHARESERVICE_PROXY_DATA_SUBSCRIBER_MANAGER_H
#define DATASHARESERVICE_PROXY_DATA_SUBSCRIBER_MANAGER_H

#include "concurrent_map.h"
#include "data_proxy_observer.h"
#include "dataproxy_handle_common.h"

namespace OHOS::DataShare {
struct ProxyDataKey {
    ProxyDataKey(const std::string &uri, const std::string &bundleName);
    bool operator<(const ProxyDataKey &rhs) const;
    bool operator>(const ProxyDataKey &rhs) const;
    bool operator<=(const ProxyDataKey &rhs) const;
    bool operator>=(const ProxyDataKey &rhs) const;
    bool operator==(const ProxyDataKey &rhs) const;
    bool operator!=(const ProxyDataKey &rhs) const;
    std::string uri;
    std::string bundleName;
};

class ProxyDataSubscriberManager {
public:
    static ProxyDataSubscriberManager &GetInstance();
    DataProxyErrorCode Add(const ProxyDataKey &key, const sptr<IProxyDataObserver> &observer,
        const std::string &bundleName, const std::string &callerAppIdentifier, const int32_t &userId);
    DataProxyErrorCode Delete(const ProxyDataKey &key, const int32_t &userId);
    void Emit(const std::vector<ProxyDataKey> &keys, const std::map<DataShareObserver::ChangeType,
        std::vector<DataShareProxyData>> &datas, const int32_t &userId);
    void Clear();

private:
struct ObserverNode {
    ObserverNode(const sptr<IProxyDataObserver> &observer, const uint32_t &callerTokenId,
        const std::string &bundleName, const std::string &callerAppIdentifier, const int32_t &userId = 0);
        sptr<IProxyDataObserver> observer;
        uint32_t callerTokenId;
        std::string bundleName;
        std::string callerAppIdentifier;
        int32_t userId = 0;
    };
    
    bool CheckAllowList(const std::vector<std::string> &allowList,
        const std::string &callerAppIdentifier);
    ProxyDataSubscriberManager() = default;
    ConcurrentMap<ProxyDataKey, std::vector<ObserverNode>> ProxyDataCache_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_PROXY_DATA_SUBSCRIBER_MANAGER_H

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

#ifndef DATASHARESERVICE_DATA_SHARE_OBS_PROXY_H
#define DATASHARESERVICE_DATA_SHARE_OBS_PROXY_H

#include "iremote_proxy.h"
#include "data_proxy_observer.h"
#include "datashare_template.h"

namespace OHOS::DataShare {
class RdbObserverProxy : public IRemoteProxy<IDataProxyRdbObserver> {
public:
    explicit RdbObserverProxy(const sptr<IRemoteObject>& remote) : IRemoteProxy<IDataProxyRdbObserver>(remote) {}
    virtual ~RdbObserverProxy() {}
    void OnChangeFromRdb(RdbChangeNode &changeNode) override;

private:
    static inline BrokerDelegator<RdbObserverProxy> delegator_;
};

class PublishedDataObserverProxy : public IRemoteProxy<IDataProxyPublishedDataObserver> {
public:
    explicit PublishedDataObserverProxy(const sptr<IRemoteObject>& remote)
        : IRemoteProxy<IDataProxyPublishedDataObserver>(remote) {}
    virtual ~PublishedDataObserverProxy() {}
    void OnChangeFromPublishedData(PublishedDataChangeNode &changeNode) override;

private:
    static inline BrokerDelegator<PublishedDataObserverProxy> delegator_;
};
} // namespace OHOS::DataShare
#endif // DATA_SHARE_OBS_PROXY_H

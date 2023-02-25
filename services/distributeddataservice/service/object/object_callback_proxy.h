/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_CALLBACK_PROXY_H
#define DISTRIBUTEDDATAMGR_OBJECT_CALLBACK_PROXY_H
#include <iremote_broker.h>
#include <iremote_proxy.h>
#include "object_callback.h"

namespace OHOS {
namespace DistributedObject {
class ObjectSaveCallbackProxyBroker : public IObjectSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectSaveCallback");
};

class ObjectSaveCallbackProxy : public IRemoteProxy<ObjectSaveCallbackProxyBroker> {
public:
    explicit ObjectSaveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectSaveCallbackProxy() = default;
    void Completed(const std::map<std::string, int32_t> &results) override;

private:
    static inline BrokerDelegator<ObjectSaveCallbackProxy> delegator_;
};

class ObjectRevokeSaveCallbackProxyBroker : public IObjectRevokeSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRevokeSaveCallback");
};

class ObjectRevokeSaveCallbackProxy : public IRemoteProxy<ObjectRevokeSaveCallbackProxyBroker> {
public:
    explicit ObjectRevokeSaveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectRevokeSaveCallbackProxy() = default;
    void Completed(int32_t status) override;

private:
    static inline BrokerDelegator<ObjectRevokeSaveCallbackProxy> delegator_;
};

class ObjectRetrieveCallbackProxyBroker : public IObjectRetrieveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRetrieveCallback");
};

class ObjectRetrieveCallbackProxy : public IRemoteProxy<ObjectRetrieveCallbackProxyBroker> {
public:
    explicit ObjectRetrieveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectRetrieveCallbackProxy() = default;
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results) override;

private:
    static inline BrokerDelegator<ObjectRetrieveCallbackProxy> delegator_;
};

class ObjectChangeCallbackProxyBroker : public IObjectChangeCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectChangeCallback");
};

class ObjectChangeCallbackProxy : public IRemoteProxy<ObjectChangeCallbackProxyBroker> {
public:
    explicit ObjectChangeCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectChangeCallbackProxy() = default;
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results) override;

private:
    static inline BrokerDelegator<ObjectChangeCallbackProxy> delegator_;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_CALLBACK_PROXY_H

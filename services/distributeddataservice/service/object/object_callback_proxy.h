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
#include "iobject_callback.h"
#include <iremote_broker.h>
#include <iremote_stub.h>
namespace OHOS {
namespace DistributedObject {
class ObjectSaveCallbackProxy : public IRemoteProxy<IObjectSaveCallback> {
public:
    explicit ObjectSaveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectSaveCallbackProxy() = default;
    void Completed(const std::map<std::string, int32_t> &results) override;

private:
    static inline BrokerDelegator<ObjectSaveCallbackProxy> delegator_;
};

class ObjectRevokeSaveCallbackProxy : public IRemoteProxy<IObjectRevokeSaveCallback> {
public:
    explicit ObjectRevokeSaveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectRevokeSaveCallbackProxy() = default;
    void Completed(int32_t status) override;

private:
    static inline BrokerDelegator<ObjectRevokeSaveCallbackProxy> delegator_;
};

class ObjectRetrieveCallbackProxy : public IRemoteProxy<IObjectRetrieveCallback> {
public:
    explicit ObjectRetrieveCallbackProxy(const sptr<IRemoteObject> &impl);
    ~ObjectRetrieveCallbackProxy() = default;
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results) override;

private:
    static inline BrokerDelegator<ObjectRetrieveCallbackProxy> delegator_;
};

class ObjectChangeCallbackProxy : public IRemoteProxy<IObjectChangeCallback> {
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

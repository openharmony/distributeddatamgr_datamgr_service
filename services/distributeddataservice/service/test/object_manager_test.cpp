/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObjectManagerTest"

#include "object_manager.h"

#include <gtest/gtest.h>
#include <ipc_skeleton.h>

#include "bootstrap.h"
#include "device_manager_adapter_mock.h"
#include "executor_pool.h"
#include "kv_store_nb_delegate_mock.h"
#include "kvstore_meta_manager.h"
#include "metadata/object_user_meta_data.h"
#include "object_types.h"
#include "snapshot/machine_status.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace std;
using namespace testing;
using AssetValue = OHOS::CommonType::AssetValue;
using RestoreStatus = OHOS::DistributedObject::ObjectStoreManager::RestoreStatus;
namespace OHOS::Test {
class IObjectSaveCallback {
public:
    virtual void Completed(const std::map<std::string, int32_t> &results) = 0;
};
class IObjectRevokeSaveCallback {
public:
    virtual void Completed(int32_t status) = 0;
};
class IObjectRetrieveCallback {
public:
    virtual void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) = 0;
};
class IObjectChangeCallback {
public:
    virtual void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) = 0;
};

class IObjectProgressCallback {
public:
    virtual void Completed(int32_t progress) = 0;
};

class ObjectSaveCallbackBroker : public IObjectSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectSaveCallback");
};
class ObjectSaveCallbackStub : public IRemoteStub<ObjectSaveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};
class ObjectRevokeSaveCallbackBroker : public IObjectRevokeSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRevokeSaveCallback");
};
class ObjectRevokeSaveCallbackStub : public IRemoteStub<ObjectRevokeSaveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};
class ObjectRetrieveCallbackBroker : public IObjectRetrieveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRetrieveCallback");
};
class ObjectRetrieveCallbackStub : public IRemoteStub<ObjectRetrieveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectChangeCallbackBroker : public IObjectChangeCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectChangeCallback");
};

class ObjectChangeCallbackStub : public IRemoteStub<ObjectChangeCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectProgressCallbackBroker : public IObjectProgressCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectProgressCallback");
};

class ObjectProgressCallbackStub : public IRemoteStub<ObjectProgressCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectSaveCallback : public ObjectSaveCallbackStub {
public:
    explicit ObjectSaveCallback(const std::function<void(const std::map<std::string, int32_t> &)> &callback)
        : callback_(callback)
    {
    }
    void Completed(const std::map<std::string, int32_t> &results) override
    {
    }

private:
    const std::function<void(const std::map<std::string, int32_t> &)> callback_;
};
class ObjectRevokeSaveCallback : public ObjectRevokeSaveCallbackStub {
public:
    explicit ObjectRevokeSaveCallback(const std::function<void(int32_t)> &callback) : callback_(callback)
    {
    }
    void Completed(int32_t) override
    {
    }

private:
    const std::function<void(int32_t status)> callback_;
};
class ObjectRetrieveCallback : public ObjectRetrieveCallbackStub {
public:
    explicit ObjectRetrieveCallback(
        const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> &callback)
        : callback_(callback)
    {
    }
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) override
    {
    }

private:
    const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> callback_;
};
} // namespace OHOS::Test

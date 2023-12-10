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

#ifndef DISTRIBUTEDDATASERVICE_OBJECT_SERVICE_H
#define DISTRIBUTEDDATASERVICE_OBJECT_SERVICE_H

#include "feature/static_acts.h"
#include "object_manager.h"
#include "object_service_stub.h"
#include "visibility.h"
#include "object_data_listener.h"

namespace OHOS::DistributedObject {
class API_EXPORT ObjectServiceImpl : public ObjectServiceStub {
public:
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;
    ObjectServiceImpl();
    virtual ~ObjectServiceImpl();
    int32_t ObjectStoreSave(const std::string &bundleName, const std::string &sessionId,
        const std::string &deviceId, const std::map<std::string, std::vector<uint8_t>> &data,
        sptr<IRemoteObject> callback) override;
    int32_t ObjectStoreRetrieve(
        const std::string &bundleName, const std::string &sessionId, sptr<IRemoteObject> callback) override;
    int32_t ObjectStoreRevokeSave(const std::string &bundleName, const std::string &sessionId,
        sptr<IRemoteObject> callback) override;
    int32_t RegisterDataObserver(const std::string &bundleName, const std::string &sessionId,
        sptr<IRemoteObject> callback) override;
    int32_t UnregisterDataChangeObserver(const std::string &bundleName, const std::string &sessionId) override;
    int32_t DeleteSnapshot(const std::string &bundleName, const std::string &sessionId) override;
    int32_t IsBundleNameEqualTokenId(
        const std::string &bundleName, const std::string &sessionId, const uint32_t &tokenId);
    void Clear();
    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override;
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId) override;
    int32_t OnInitialize() override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    int32_t OnBind(const BindInfo &bindInfo) override;
    void DumpObjectServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    int32_t OnAssetChanged(const std::string &bundleName, const std::string &sessionId,
        const std::string &deviceId, const ObjectStore::Asset &assetValue) override;
    int32_t BindAssetStore(const std::string &bundleName, const std::string &sessionId,
        ObjectStore::Asset &asset, ObjectStore::AssetBindInfo &bindInfo) override;
private:
    using StaticActs = DistributedData::StaticActs;
    class ObjectStatic : public StaticActs {
    public:
        ~ObjectStatic() override {};
        int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override;
    };
    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<ObjectStatic> staticActs_;
    };
    void RegisterObjectServiceInfo();
    void RegisterHandler();
    static Factory factory_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedObject
#endif

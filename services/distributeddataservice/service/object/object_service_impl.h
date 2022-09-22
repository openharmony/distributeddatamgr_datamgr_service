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

#include "object_manager.h"
#include "object_service_stub.h"
#include "visibility.h"
#include "object_data_listener.h"

namespace OHOS::DistributedObject {
class API_EXPORT ObjectServiceImpl : public ObjectServiceStub {
public:
    ObjectServiceImpl();

    void Initialize();
    int32_t ObjectStoreSave(const std::string &bundleName, const std::string &sessionId,
        const std::string &deviceId, const std::map<std::string, std::vector<uint8_t>> &data,
        sptr<IObjectSaveCallback> callback) override;
    int32_t ObjectStoreRetrieve(
        const std::string &bundleName, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback) override;
    int32_t ObjectStoreRevokeSave(const std::string &bundleName, const std::string &sessionId,
        sptr<IObjectRevokeSaveCallback> callback) override;
    int32_t RegisterDataObserver(const std::string &bundleName, const std::string &sessionId,
        sptr<IObjectChangeCallback> callback) override;
    int32_t VerifyPermissionByBundleName(
        const std::string &bundleName, const std::string &sessionId, const uint32_t &tokenId);
    void Clear();
    int32_t DeleteByAppId(const std::string &appId);
    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param);
};
} // namespace OHOS::DistributedObject
#endif

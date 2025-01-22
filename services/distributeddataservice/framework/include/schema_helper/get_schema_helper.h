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
#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_GET_SCHEMA_HELPER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_GET_SCHEMA_HELPER_H

#include <memory>
#include <string>

#include "iremote_object.h"
#include "visibility.h"

namespace OHOS {
namespace AppExecFwk {
class IBundleMgr;
}
} // namespace OHOS

namespace OHOS::DistributedData {
struct AppInfo {
    std::string bundleName;
    int32_t userId;
    int32_t appIndex;
};

class GetSchemaHelper final : public std::enable_shared_from_this<GetSchemaHelper> {
public:
    API_EXPORT static GetSchemaHelper &GetInstance();
    API_EXPORT std::vector<std::string> GetSchemaFromHap(const std::string &schemaPath, const AppInfo &info);

private:
    GetSchemaHelper() = default;
    ~GetSchemaHelper();
    class ServiceDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit ServiceDeathRecipient(std::weak_ptr<GetSchemaHelper> owner) : owner_(owner)
        {
        }
        void OnRemoteDied(const wptr<IRemoteObject> &object) override
        {
            auto owner = owner_.lock();
            if (owner != nullptr) {
                owner->OnRemoteDied();
            }
        }

    private:
        std::weak_ptr<GetSchemaHelper> owner_;
    };
    void OnRemoteDied();
    sptr<AppExecFwk::IBundleMgr> GetBundleMgr();
    std::mutex mutex_;
    sptr<IRemoteObject> object_;
    sptr<GetSchemaHelper::ServiceDeathRecipient> deathRecipient_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_GET_SCHEMA_HELPER_H
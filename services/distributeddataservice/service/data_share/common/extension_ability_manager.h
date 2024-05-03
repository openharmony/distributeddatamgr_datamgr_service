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

#ifndef DATASHARESERVICE_EXTENSION_ABILITY_MANAGER_H
#define DATASHARESERVICE_EXTENSION_ABILITY_MANAGER_H

#include "concurrent_map.h"
#include "executor_pool.h"
#include "iremote_object.h"

namespace OHOS::DataShare {
class ExtensionAbilityManager {
public:
    static ExtensionAbilityManager &GetInstance();
    void SetExecutorPool(std::shared_ptr<ExecutorPool> executor);
    int32_t ConnectExtension(const std::string &uri, const std::string &bundleName,
        const sptr<IRemoteObject> &callback);
    void DelayDisconnect(const std::string &bundleName);

private:
    std::shared_ptr<ExecutorPool> executor_ = nullptr;
    ConcurrentMap<std::string, sptr<IRemoteObject>> connectCallbackCache_;
    static constexpr int WAIT_DISCONNECT_TIME = 5;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_EXTENSION_ABILITY_MANAGER_H

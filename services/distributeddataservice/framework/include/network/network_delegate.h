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

#ifndef DISTRIBUTEDDATAMGR_NETWORK_DELEGATE_H
#define DISTRIBUTEDDATAMGR_NETWORK_DELEGATE_H

#include <memory>
#include <mutex>

#include "executor_pool.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedData {
class NetworkDelegate {
public:
    enum NetworkType {
        NONE,
        CELLULAR,
        WIFI,
        ETHERNET,
        OTHER
    };
    API_EXPORT virtual ~NetworkDelegate() = default;
    API_EXPORT static NetworkDelegate *GetInstance();
    API_EXPORT static bool RegisterNetworkInstance(NetworkDelegate *instance);
    virtual bool IsNetworkAvailable() = 0;
    virtual void RegOnNetworkChange() = 0;
    virtual void BindExecutor(std::shared_ptr<ExecutorPool> executors) = 0;
    virtual NetworkType GetNetworkType(bool retrieve = false) = 0;

private:
    static NetworkDelegate *instance_;
};
} // namespace DistributedKv
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_NETWORK_DELEGATE_H
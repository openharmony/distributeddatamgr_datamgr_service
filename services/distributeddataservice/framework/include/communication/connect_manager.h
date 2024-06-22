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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMUNICATION_CONNECT_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMUNICATION_CONNECT_MANAGER_H

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include "concurrent_map.h"
#include "visibility.h"
namespace OHOS {
namespace AppDistributedKv {
class API_EXPORT ConnectManager {
public:
    using ConnectTask = std::function<void(void)>;
    using CloseSessionTask = std::function<bool(const std::string &networkId)>;
    using SessionCloseListener = std::function<void(const std::string &networkId)>;
    using SessionOpenListener = std::function<void(const std::string &networkId)>;

    API_EXPORT static std::shared_ptr<ConnectManager> GetInstance();
    API_EXPORT static bool RegisterInstance(std::shared_ptr<ConnectManager> instance);

    API_EXPORT static bool CloseSession(const std::string &networkId);
    API_EXPORT static bool RegisterCloseSessionTask(CloseSessionTask task);

    API_EXPORT static bool RegisterSessionCloseListener(const std::string &name, SessionCloseListener listener);
    API_EXPORT static void UnRegisterSessionCloseListener(const std::string &name);
    API_EXPORT static void OnSessionClose(const std::string &networkId);

    API_EXPORT static bool RegisterSessionOpenListener(const std::string &name, SessionOpenListener listener);
    API_EXPORT static void UnRegisterSessionOpenListener(const std::string &name);
    API_EXPORT static void OnSessionOpen(const std::string &networkId);

    ConnectManager() = default;
    virtual ~ConnectManager() = default;

    virtual void OnStart();
    virtual void OnDestory();
    virtual int32_t ApplyConnect(const std::string &networkId, ConnectTask task);

private:
    static std::mutex mtx_;
    static std::shared_ptr<ConnectManager> instance_;
    static CloseSessionTask closeSessionTask_;
    static ConcurrentMap<std::string, SessionCloseListener> sessionCloseListener_;
    static ConcurrentMap<std::string, SessionOpenListener> sessionOpenListener_;
};
} // namespace AppDistributedKv
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_COMMUNICATION_CONNECT_MANAGER_H
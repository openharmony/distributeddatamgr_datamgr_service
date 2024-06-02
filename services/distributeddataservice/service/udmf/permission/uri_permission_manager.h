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

#ifndef UDMF_URI_PERMISSION_MANAGER_H
#define UDMF_URI_PERMISSION_MANAGER_H

#include <memory>
#include <string>

#include "concurrent_map.h"
#include "error_code.h"
#include "executor_pool.h"
#include "uri.h"

namespace OHOS {
namespace UDMF {
class UriPermissionManager {
public:
    using Time = std::chrono::steady_clock::time_point;
    static UriPermissionManager &GetInstance();
    Status GrantUriPermission(
        const std::vector<Uri> &allUri, const std::string &bundleName, const std::string &queryKey);
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);

private:
    UriPermissionManager() {}
    ~UriPermissionManager() {}
    UriPermissionManager(const UriPermissionManager &mgr) = delete;
    UriPermissionManager &operator=(const UriPermissionManager &mgr) = delete;

    void RevokeUriPermission();

    ConcurrentMap<std::string, Time> stores_;
    static constexpr int64_t INTERVAL = 60;  // 60 min
    const std::string delimiter_ = "||";
    ExecutorPool::TaskId taskId_ = ExecutorPool::INVALID_TASK_ID;
    std::mutex taskMutex_;
    std::shared_ptr<ExecutorPool> executorPool_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_URI_PERMISSION_MANAGER_H
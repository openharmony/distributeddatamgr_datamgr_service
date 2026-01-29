/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATA_QOS_MANAGER_H
#define DISTRIBUTEDDATA_QOS_MANAGER_H

#include <chrono>
#include <functional>

#include "visibility.h"
namespace OHOS {
namespace QOS {
enum class QosLevel : int;
} // namespace QOS

namespace DistributedData {

class API_EXPORT QosManager {
public:
    using SetQosFunc = std::function<int(QOS::QosLevel)>;
    using ResetQosFunc = std::function<int()>;

    explicit QosManager(bool isDataShare = false);
    ~QosManager();

    QosManager(const QosManager&) = delete;
    QosManager& operator=(const QosManager&) = delete;
    QosManager(QosManager&&) = delete;
    QosManager& operator=(QosManager&&) = delete;

    static void SetQosFunctions(SetQosFunc setFunc, ResetQosFunc resetFunc);

    // Test-only method to set startup time for testing
    static void SetStartTimeForTest(const std::chrono::steady_clock::time_point& time);

    // Test-only method to reset thread-local flag
    static void ResetForTest();

private:
    bool IsInStartupPhase() const;

    bool isDataShare_;

    static SetQosFunc setQosFunc_;
    static ResetQosFunc resetQosFunc_;
};

} // namespace DistributedData
} // namespace OHOS

#endif // DISTRIBUTEDDATA_QOS_MANAGER_H

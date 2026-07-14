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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORK_DFX_XCOLLIE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORK_DFX_XCOLLIE_H

#include <cstdint>
#include <functional>
#include <string>

#include "visibility.h"

namespace OHOS::DistributedData {
class XCollieDelegate {
public:
    API_EXPORT virtual ~XCollieDelegate() = default;
    API_EXPORT virtual int32_t SetTimer(const std::string &tag, uint32_t timeoutSeconds,
        std::function<void(void *)> func, void *arg, uint32_t flag) = 0;
    API_EXPORT virtual void CancelTimer(int32_t id) = 0;
    static XCollieDelegate *GetInstance();
    API_EXPORT static bool RegisterXCollieInstance(XCollieDelegate *instance);

private:
    static XCollieDelegate *instance_;
};

class XCollie final {
public:
    enum FLAG { XCOLLIE_LOG = 0x1, XCOLLIE_RECOVERY = 0x2 };

    API_EXPORT XCollie(const std::string &tag, uint32_t flag, uint32_t timeoutSeconds = RESTART_TIME_THRESHOLD,
        std::function<void(void *)> func = nullptr, void *arg = nullptr);
    API_EXPORT ~XCollie();

    XCollie(const XCollie &other) = delete;
    XCollie &operator=(const XCollie &other) = delete;
    XCollie(XCollie &&other) = delete;
    XCollie &operator=(XCollie &&other) = delete;

private:
    int32_t id_ = -1;
    static constexpr uint32_t RESTART_TIME_THRESHOLD = 30;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORK_DFX_XCOLLIE_H

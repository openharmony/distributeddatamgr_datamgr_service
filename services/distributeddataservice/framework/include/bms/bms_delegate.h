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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORK_BMS_DELEGATE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORK_BMS_DELEGATE_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "visibility.h"

namespace OHOS::DistributedData {
class BmsDelegate {
public:
    API_EXPORT static std::shared_ptr<BmsDelegate> GetInstance();
    API_EXPORT static bool RegisterInstance(std::shared_ptr<BmsDelegate> instance);
    BmsDelegate() = default;
    API_EXPORT virtual ~BmsDelegate() = default;
    API_EXPORT virtual std::string GetCallerAppIdentifier(const std::string &bundleName, int32_t userId);

private:
    static std::mutex mutex_;
    static std::shared_ptr<BmsDelegate> instance_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORK_BMS_DELEGATE_H

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

#ifndef OHOS_SENSITIVE_H
#define OHOS_SENSITIVE_H

#include <string>
#include "visibility.h"
#include "iprocess_system_api_adapter.h"
#include "dev_slinfo_mgr.h"

namespace OHOS {
namespace DistributedKv {
class Sensitive final {
public:
    explicit Sensitive(std::string deviceId);
    explicit Sensitive();
    Sensitive(const Sensitive &sensitive);
    Sensitive &operator=(const Sensitive &sensitive);
    Sensitive(Sensitive &&sensitive) noexcept;
    Sensitive &operator=(Sensitive &&sensitive) noexcept;
    ~Sensitive() = default;
    operator bool () const;
    bool operator >= (const DistributedDB::SecurityOption &option);
    uint32_t GetDeviceSecurityLevel();

private:
    uint32_t GetSensitiveLevel(const std::string &udid);
    bool InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid);
    std::string deviceId {};
    uint32_t securityLevel = 0;
};
} // namespace DistributedKv
} // namespace OHOS
#endif // OHOS_SENSITIVE_H
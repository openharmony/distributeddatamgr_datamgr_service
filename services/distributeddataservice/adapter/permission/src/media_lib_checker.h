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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_DISTRIBUTEDDATASERVICE_ADAPTER_PERMISSION_SRC_MEDIA_LIB_CHECKER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_DISTRIBUTEDDATASERVICE_ADAPTER_PERMISSION_SRC_MEDIA_LIB_CHECKER_H

#include "checker/checker_manager.h"
namespace OHOS {
namespace DistributedData {
class MediaLibChecker : public CheckerManager::Checker {
public:
    MediaLibChecker() noexcept;
    ~MediaLibChecker();
    void Initialize() override;
    bool SetTrustInfo(const DistributedData::CheckerManager::Trust &trust) override;
    std::string GetAppId(pid_t uid, const std::string &bundleName) override;
    bool IsValid(pid_t uid, const std::string &bundleName) override;
private:
    static MediaLibChecker instance_;
    static constexpr pid_t SYSTEM_UID = 10000;
    std::map<std::string, std::string> trusts_;
};
} // namespace DistributedData
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_DISTRIBUTEDDATASERVICE_ADAPTER_PERMISSION_SRC_MEDIA_LIB_CHECKER_H

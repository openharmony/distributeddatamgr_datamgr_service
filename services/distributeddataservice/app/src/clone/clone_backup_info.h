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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_BACKUP_INFO_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_BACKUP_INFO_H
#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
struct CloneEncryptionInfo final : public Serializable {
    std::string symkey;
    std::string algName;
    std::string iv;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
    ~CloneEncryptionInfo();
};

struct CloneBundleInfo final : public Serializable {
    std::string bundleName;
    uint32_t accessTokenId;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

struct CloneBackupInfo final : public Serializable {
    CloneEncryptionInfo encryptionInfo;
    std::vector<CloneBundleInfo> bundleInfos;
    std::string userId;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

struct CloneReplyResult final : public Serializable {
    std::string type = "ErrorInfo";
    std::string errorCode;
    std::string errorInfo;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};

struct CloneReplyCode final : public Serializable {
    std::vector<CloneReplyResult> resultInfo;
    API_EXPORT bool Marshal(json &node) const override;
    API_EXPORT bool Unmarshal(const json &node) override;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_CLONE_BACKUP_INFO_H

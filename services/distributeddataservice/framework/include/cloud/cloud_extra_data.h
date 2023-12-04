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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE5_CLOUD_EXTRA_DATA_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE5_CLOUD_EXTRA_DATA_H

#include "metadata/store_meta_data.h"
#include "serializable/serializable.h"

namespace OHOS::DistributedData {
class API_EXPORT ExtensionInfo final : public Serializable {
public:
    std::string accountId;
    std::string bundleName;
    std::string containerName;
    std::string databaseScopes;
    std::vector<std::string> scopes;
    std::string recordTypes;
    std::vector<std::string> tables;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

class API_EXPORT ExtraData final : public Serializable {
public:
    std::string header;
    std::string data;
    ExtensionInfo info;

    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    bool isPrivate() const;
    bool isShared() const;

    static constexpr const char *PRIVATE_TABLE = "private";
    static constexpr const char *SHARED_TABLE = "shared";
};
}
#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE5_CLOUD_EXTRA_DATA_H
